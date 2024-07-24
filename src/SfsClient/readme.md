## SfsClient

Do not change code under the sfs-client directory; it contains sfs-client source code from commit [6ab78af](https://github.com/microsoft/sfs-client/commits/6ab78af).  
It is created using git subtree command:
```
    git subtree add --prefix=src/SfsClient/sfs-client https://github.com/microsoft/sfs-client.git 6ab78af61bc859461ea8298786d87f24b49e3ec2 --squash
```

### Update
To update, run the following command, then update the above commit for reference.  'master' can be replaced with the appropriate commit spec as desired.
```
    git subtree pull -P src/SfsClient/sfs-client https://github.com/microsoft/sfs-client master --squash
```
**When committing the PR, DO NOT squash it.  The two commits are needed as is to allow for future subtree pulls.**
