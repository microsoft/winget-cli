## SfsClient

Do not change code under the sfs-client directory; it contains sfs-client source code from commit [c639a50](https://github.com/microsoft/sfs-client/commit/c639a50).  
It is created using git subtree command:
```
    git subtree add --prefix=src/SfsClient/sfs-client https://github.com/microsoft/sfs-client.git c639a506e712dbd29ca7ca0c78d5216658e78748 --squash
```

### Update
To update, run the following command, then update the above commit for reference.  'master' can be replaced with the appropriate commit spec as desired.
```
    git subtree pull -P src/SfsClient/sfs-client https://github.com/microsoft/sfs-client master --squash
```
**When committing the PR, DO NOT squash it.  The two commits are needed as is to allow for future subtree pulls.**
