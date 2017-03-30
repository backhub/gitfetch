#include "gitfetch.h"

int create_remote_mirror(git_remote **out, git_repository *repository, const char *name, const char *url, void *payload);

VALUE method_clone(VALUE self, VALUE remote_url, VALUE path, VALUE access_token) {
  int error;
  struct credentials_s credentials = { NULL, 0 };

  git_repository *repository;

  git_clone_options clone_options = GIT_CLONE_OPTIONS_INIT;
  clone_options.bare = true;
  clone_options.fetch_opts.download_tags = GIT_REMOTE_DOWNLOAD_TAGS_ALL;
  clone_options.remote_cb = create_remote_mirror;

  if (access_token != Qnil) {
    credentials.access_token = StringValuePtr(access_token);
    clone_options.fetch_opts.callbacks.credentials = cb_cred_access_token;
    clone_options.fetch_opts.callbacks.payload     = &credentials;
  }

  error = git_clone(&repository, StringValuePtr(remote_url), StringValuePtr(path), &clone_options);

  if (error == 0) {
    git_repository_free(repository);
  }

  if (error < 0) {
    raise_exception(error);
  }

  return Qnil;
}

int create_remote_mirror(git_remote **out, git_repository *repo, const char *name, const char *url, void *payload) {
  GIT_UNUSED(payload);

  int error;
  git_config *cfg;
  char *mirror_config;

  /* Create the repository with a mirror refspec */
  if ((error = git_remote_create_with_fetchspec(out, repo, name, url, "+refs/*:refs/*")) < 0)
    return error;

  /* Set the mirror setting to true on this remote */
  if ((error = git_repository_config(&cfg, repo)) < 0)
    return error;

  if (asprintf(&mirror_config, "remote.%s.mirror", name) == -1) {
    giterr_set_str(GITERR_OS, "asprintf failed");
    git_config_free(cfg);
    return -1;
  }

  error = git_config_set_bool(cfg, mirror_config, true);

  free(mirror_config);
  git_config_free(cfg);

  return error;
}
