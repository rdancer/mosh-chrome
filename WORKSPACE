workspace(name = "mosh_chrome")

new_http_archive(
    name = "nacl_sdk",
    # There are hardlinks in the stock archive, and Bazel can't handle them. It
    # just creates files of zero size. To work around this, I've published a
    # copy of the tarball without hardlinks. I did so by untarring the archive,
    # and tarring it back up like so:
    #     $ tar -czf naclsdk_linux.tar.gz --hard-dereference pepper_49/
    url = "https://storage.googleapis.com/mosh-chrome/naclsdk_linux_49.0.2623.87_no_hardlinks.tar.gz",
    sha256 = "d966fac06a4c3c978ac3904734aff2ea447b77b5280dd0d73a7ca2d776f8c426",

    # Here is the original config for reference.
    #url = "https://storage.googleapis.com/nativeclient-mirror/nacl/nacl_sdk/49.0.2623.87/naclsdk_linux.tar.bz2",
    #sha256 = "c53c14e5eaf6858e5b4a4e964c84d7774f03490be7986ab07c6792e807e05f14",

    strip_prefix = "pepper_49",
    build_file = "external_builds/BUILD.nacl_sdk",
)

new_http_archive(
    name = "jsoncpp",
    url = "https://gsdview.appspot.com/webports/builds/pepper_49/trunk-785-g807a23e/packages/jsoncpp_1.6.1_pnacl.tar.bz2",
    sha256 = "eb6067b5fc463b3a3777365c9a0cb3da69acd1cffbd22f8e9b1a9eb630c0d1c8",
    strip_prefix = "payload",
    build_file = "external_builds/BUILD.jsoncpp",
)

new_http_archive(
    name = "ncurses",
    url = "https://gsdview.appspot.com/webports/builds/pepper_49/trunk-785-g807a23e/packages/ncurses_5.9_pnacl.tar.bz2",
    sha256 = "5d654df56d146d593bdc855231d61f745460b182be83c4f081e824362042c6d7",
    strip_prefix = "payload",
    build_file = "external_builds/BUILD.ncurses",
)

new_http_archive(
    name = "openssl",
    url = "https://gsdview.appspot.com/webports/builds/pepper_49/trunk-785-g807a23e/packages/openssl_1.0.2e_pnacl.tar.bz2",
    sha256 = "8ff02228adbd45c22a36611ffd0d47115d62e08bdfb5750977195cf19cb2b912",
    strip_prefix = "payload",
    build_file = "external_builds/BUILD.openssl",
)

new_http_archive(
    name = "protobuf",
    url = "https://gsdview.appspot.com/webports/builds/pepper_49/trunk-785-g807a23e/packages/protobuf_3.0.0-beta-2_pnacl.tar.bz2",
    sha256 = "76780b202c2b692bbeef4b8ed5706563b9d5f02acb98d7bd6250fa00e2d6e1f9",
    strip_prefix = "payload",
    build_file = "external_builds/BUILD.protobuf",
)

new_http_archive(
    name = "zlib",
    url = "https://gsdview.appspot.com/webports/builds/pepper_49/trunk-785-g807a23e/packages/zlib_1.2.8_pnacl.tar.bz2",
    sha256 = "99203f49edad39570a3362e72373ea1d63a97b9b1828b75e45d3e856d2832033",
    strip_prefix = "payload",
    build_file = "external_builds/BUILD.zlib",
)

new_http_archive(
    name = "glibc_compat",
    url = "https://gsdview.appspot.com/webports/builds/pepper_49/trunk-785-g807a23e/packages/glibc-compat_0.1_pnacl.tar.bz2",
    sha256 = "3153933c99f161c41eb9205d7b30459fd8b367a67dcf942829182b2c15f541fd",
    strip_prefix = "payload",
    build_file = "external_builds/BUILD.glibc_compat",
)

new_git_repository(
    name = "libssh",
    remote = "https://github.com/rpwoodbu/libssh.git",
    commit = "f0a54a34837ad044571b1f9375309f7c0bd19c52",
    build_file = "external_builds/BUILD.libssh",
)

new_git_repository(
    name = "mosh",
    remote = "https://github.com/rpwoodbu/mosh.git",
    commit = "3c3b356cb5e387887499beb2eedddce185f36944",
    build_file = "external_builds/BUILD.mosh",
)

# Using a GitHub mirror as a workaround for the fact that the libapps repo
# causes "not authorized" from Bazel. I assume that's a Bazel bug?
new_git_repository(
    name = "libapps",
    remote = "https://github.com/rpwoodbu/libapps.git",
    commit = "2e1688583443d80e4e512c41847eb9fd29fb0cb2", # hterm 1.54
    build_file = "external_builds/BUILD.libapps",
)

# Bazel repo gives us access to useful things, e.g., //third_party.
git_repository(
    name = "io_bazel",
    remote = "https://github.com/bazelbuild/bazel.git",
    commit = "e671d2950fb56c499db2c99a3d6dad2d291ed873", # tag = "0.3.0",
)
