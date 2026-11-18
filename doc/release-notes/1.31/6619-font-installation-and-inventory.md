## Bug Fixes

### Minor Bug Fixes

* Fixed font installation failures when different files in a package select the same collision-adjusted filename.
* Fixed installation, repair, and removal of fonts with filenames longer than 128 characters.
* Fixed font filename collisions exceeding the Windows filename length limit during installation and repair.
* Fixed malformed font package registrations preventing other packages or versions from appearing in detailed font listings.
* Improved font listing, package enumeration, and source indexing performance by reusing registry data and indexing filenames.
