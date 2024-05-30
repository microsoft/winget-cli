## SfsClient

Do not change code under the sfs-client directory; it contains sfs-client source code from commit [ff315ec](https://github.com/microsoft/sfs-client/commits/ff315ec).  
It is created using git subtree command:
```
    git subtree add --prefix=src/SfsClient/sfs-client https://github.com/microsoft/sfs-client.git cf18b357f43aa9bbaba7d8b3b3774b39140aa00f --squash
```

### Update
To update, run the following command, then update the above commit for reference.  'master' can be replaced with the appropriate commit spec as desired.
```
    git subtree pull -P src/SfsClient/sfs-client https://github.com/microsoft/sfs-client master --squash
```
**When committing the PR, DO NOT squash it.  The two commits are needed as is to allow for future subtree pulls.**
