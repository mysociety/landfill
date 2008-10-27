Just edit the code to make patches, and commit.

Put new versions of upstream in a new directory.

Do diffs in source control of previous version to find changes that
were made to that, and apply them to the new version. Useful to make a file,
such as mysociety-changes-as-of-2007-05-13.patch whenever you upgrade to a new
version.

Will need to update ../bin/compile-cvstrac to refer to new directory.

