// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RestSource.h"
#include "RestSourceFactory.h"
#include "cpprest/http_client.h"

namespace AppInstaller::Repository::Rest
{
    RestSource::RestSource(const SourceDetails& details, std::string identifier, RestClient&& restClient)
        : m_details(details), m_identifier(std::move(identifier)), m_restClient(std::move(restClient))
    {
    }

    const SourceDetails& RestSource::GetDetails() const
    {
        return m_details;
    }

    const std::string& RestSource::GetIdentifier() const
    {
        return m_identifier;
    }

    SearchResult RestSource::Search(const SearchRequest& request) const
    {
        // Make search for everything work
        RestClient::SearchResult results = m_restClient.Search(request);
        SearchResult returnVal;

        /*web::http::client::http_client client(L"https://jsonplaceholder.typicode.com/posts");
        client.request(web::http::methods::GET, U("/"))
            .then([](const web::http::http_response& response) {
            try {
                std::cout << response.status_code();
            }
            catch (const web::http::http_exception& e) {
                std::cout << "error " << e.what() << std::endl;
            }
                }).wait();*/

        return returnVal;
    }
}
