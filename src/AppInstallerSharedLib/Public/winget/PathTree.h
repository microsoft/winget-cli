// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <wil/result.h>
#include <functional>
#include <map>
#include <queue>

namespace AppInstaller::Filesystem
{
    // Container that holds a map of items addressable by path.
    template <typename Value>
    struct PathTree
    {
        using value_t = Value;

        PathTree() = default;

    private:
        struct Node
        {
            value_t Value{};
            std::map<std::filesystem::path, Node> Children;
        };

    public:
        // Returns the value for the given path, inserting it if necessary.
        value_t& FindOrInsert(const std::filesystem::path& path)
        {
            return FindNode(path, true)->Value;
        }

        // Finds the value for the given path; returns null if not found.
        value_t* Find(const std::filesystem::path& path)
        {
            Node* node = FindNode(path, false);
            return node ? &node->Value : nullptr;
        }

        // Finds the value for the given path; returns null if not found.
        const value_t* Find(const std::filesystem::path& path) const
        {
            const Node* node = FindNode(path);
            return node ? &node->Value : nullptr;
        }

        // Invokes the `visit` function for each value in the tree starting at `initialPath` (unconditionally)
        // and recursively continuing on to children for whom the predicate returns true.
        void VisitIf(const std::filesystem::path& initialPath, std::function<void(const value_t&)> visit, std::function<bool(const value_t&)> predicate) const
        {
            const Node* node = FindNode(initialPath);
            if (node)
            {
                std::queue<const Node*> nodes;
                nodes.push(node);

                while (!nodes.empty())
                {
                    const Node* currentNode = nodes.front();
                    nodes.pop();

                    visit(currentNode->Value);

                    for (const auto& child : currentNode->Children)
                    {
                        if (predicate(child.second.Value))
                        {
                            nodes.push(&child.second);
                        }
                    }
                }
            }
        }

    private:
        // Finds the node for the given path, creating as needed if requested.
        Node* FindNode(const std::filesystem::path& path, bool createIfNeeded)
        {
            if (path.empty())
            {
                if (createIfNeeded)
                {
                    THROW_HR(E_INVALIDARG);
                }
                else
                {
                    return nullptr;
                }
            }

            const auto& nodePath = std::filesystem::weakly_canonical(path);
            Node* currentNode = &m_rootNode;

            for (const auto& pathPart : nodePath)
            {
                auto& children = currentNode->Children;

                if (createIfNeeded)
                {
                    currentNode = &children[pathPart];
                }
                else
                {
                    auto itr = children.find(pathPart);

                    if (itr != children.end())
                    {
                        currentNode = &itr->second;
                    }
                    else
                    {
                        // Not found and should not create
                        return nullptr;
                    }
                }
            }

            return currentNode;
        }

        // Finds the node for the given path; returns null if not found.
        const Node* FindNode(const std::filesystem::path& path) const
        {
            return const_cast<PathTree*>(this)->FindNode(path, false);
        }

        Node m_rootNode;
    };
}
