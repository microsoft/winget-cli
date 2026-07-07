FROM mcr.microsoft.com/devcontainers/dotnet:1-8.0-bookworm

# Install SQL Tools: SQLPackage and sqlcmd
COPY mssql/installSQLtools.sh installSQLtools.sh
RUN bash ./installSQLtools.sh \
     && apt-get clean -y && rm -rf /var/lib/apt/lists/* /tmp/library-scripts

# [Optional] Uncomment this section to install additional OS packages.
# RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
#     && apt-get -y install --no-install-recommends <your-package-list-here>

# [Optional] Uncomment this line to install global node packages.
RUN su vscode -c "source /usr/local/share/nvm/nvm.sh && nvm install --lts && nvm use --lts && npm install -g typescript" 2>&1
# RUN su vscode -c "source /usr/local/share/nvm/nvm.sh && npm install -g <your-package-here>" 2>&1
