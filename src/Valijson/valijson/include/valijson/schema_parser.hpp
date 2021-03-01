#pragma once

#include <stdexcept>
#include <iostream>
#include <vector>
#include <memory>
#include <functional>

#include <valijson/adapters/adapter.hpp>
#include <valijson/constraints/concrete_constraints.hpp>
#include <valijson/internal/debug.hpp>
#include <valijson/internal/json_pointer.hpp>
#include <valijson/internal/json_reference.hpp>
#include <valijson/internal/uri.hpp>
#include <valijson/constraint_builder.hpp>
#include <valijson/schema.hpp>

#ifdef __clang__
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wunused-local-typedef"
#endif

namespace valijson {

/**
 * @brief  Parser for populating a Schema based on a JSON Schema document.
 *
 * The SchemaParser class supports Drafts 3 and 4 of JSON Schema, however
 * Draft 3 support should be considered deprecated.
 *
 * The functions provided by this class have been templated so that they can
 * be used with different Adapter types.
 */
class SchemaParser
{
public:
    /// Supported versions of JSON Schema
    enum Version {
        kDraft3,      ///< @deprecated JSON Schema v3 has been superseded by v4
        kDraft4,
        kDraft7
    };

    /// Version of JSON Schema that should be expected when parsing
    const Version version;

    /**
     * @brief  Construct a new SchemaParser for a given version of JSON Schema
     *
     * @param  version  Version of JSON Schema that will be expected
     */
    SchemaParser(const Version version = kDraft7)
      : version(version) { }

    /**
     * @brief  Release memory associated with custom ConstraintBuilders
     */
    ~SchemaParser()
    {
        for (auto entry : constraintBuilders) {
            delete entry.second;
        }
    }

    /**
     * @brief  Struct to contain templated function type for fetching documents
     */
    template<typename AdapterType>
    struct FunctionPtrs
    {
        typedef typename adapters::AdapterTraits<AdapterType>::DocumentType
                DocumentType;

        /// Templated function pointer type for fetching remote documents
        typedef std::function< const DocumentType* (const std::string &uri) >  FetchDoc ;

        /// Templated function pointer type for freeing fetched documents
        typedef std::function< void (const DocumentType *)> FreeDoc ;
    };

    /**
     * @brief  Add a custom contraint to this SchemaParser

     * @param  key      name that will be used to identify relevant constraints
     *                  while parsing a schema document
     * @param  builder  pointer to a subclass of ConstraintBuilder that can
     *                  parse custom constraints found in a schema document,
     *                  and return an appropriate instance of Constraint; this
     *                  class guarantees that it will take ownership of this
     *                  pointer - unless this function throws an exception
     *
     * @todo   consider accepting a list of custom ConstraintBuilders in
     *         constructor, so that this class remains immutable after
     *         construction
     *
     * @todo   Add additional checks for key conflicts, empty keys, and
     *         potential restrictions relating to case sensitivity
     */
    void addConstraintBuilder(const std::string &key,
            const ConstraintBuilder *builder)
    {
        constraintBuilders.push_back(std::make_pair(key, builder));
    }

    /**
     * @brief  Populate a Schema object from JSON Schema document
     *
     * When processing Draft 3 schemas, the parentSubschema and ownName pointers
     * should be set in contexts where a 'required' constraint would be valid.
     * These are used to add a RequiredConstraint object to the Schema that
     * contains the required property.
     *
     * @param  node          Reference to node to parse
     * @param  schema        Reference to Schema to populate
     * @param  fetchDoc      Function to fetch remote JSON documents (optional)
     */
    template<typename AdapterType>
    void populateSchema(
        const AdapterType &node,
        Schema &schema,
        typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc = nullptr ,
        typename FunctionPtrs<AdapterType>::FreeDoc freeDoc = nullptr )
    {
        if ((fetchDoc == nullptr ) ^ (freeDoc == nullptr)) {
            throw std::runtime_error(
                    "Remote document fetching cannot be enabled without both "
                    "fetch and free functions");
        }

        typename DocumentCache<AdapterType>::Type docCache;
        SchemaCache schemaCache;
        try {
            resolveThenPopulateSchema(schema, node, node, schema,
                    opt::optional<std::string>(), "",
                    fetchDoc, nullptr, nullptr, docCache, schemaCache);
        } catch (...) {
            freeDocumentCache<AdapterType>(docCache, freeDoc);
            throw;
        }

        freeDocumentCache<AdapterType>(docCache, freeDoc);
    }

private:

    typedef std::vector<std::pair<std::string, const ConstraintBuilder *> >
        ConstraintBuilders;

    ConstraintBuilders constraintBuilders;

    template<typename AdapterType>
    struct DocumentCache
    {
        typedef typename adapters::AdapterTraits<AdapterType>::DocumentType
                DocumentType;

        typedef std::map<std::string, const DocumentType*> Type;
    };

    typedef std::map<std::string, const Subschema *> SchemaCache;

    /**
     * @brief  Free memory used by fetched documents
     *
     * If a custom 'free' function has not been provided, then the default
     * delete operator will be used.
     *
     * @param  docCache  collection of fetched documents to free
     * @param  freeDoc   optional custom free function
     */
    template<typename AdapterType>
    void freeDocumentCache(const typename DocumentCache<AdapterType>::Type
            &docCache, typename FunctionPtrs<AdapterType>::FreeDoc freeDoc)
    {
        typedef typename DocumentCache<AdapterType>::Type DocCacheType;

        for (const typename DocCacheType::value_type &v : docCache) {
            freeDoc(v.second);
        }
    }

    /**
     * @brief  Find the absolute URI for a document, within a resolution scope
     *
     * This function captures five different cases that can occur when
     * attempting to resolve a document URI within a particular resolution
     * scope:
     *
     *  - resolution scope not present, but absolute document URI is
     *       => document URI as-is
     *  - resolution scope not present, and document URI is relative or absent
     *       => no result
     *  - resolution scope is present, and document URI is a relative path
     *       => resolve document URI relative to resolution scope
     *  - resolution scope is present, and document URI is absolute
     *       => document URI as-is
     *  - resolution scope is present, but document URI is not
     *       => resolution scope as-is
     *
     * This function assumes that the resolution scope is absolute.
     *
     * When resolving a document URI relative to the resolution scope, the
     * document URI should be used to replace the path, query and fragment
     * portions of URI provided by the resolution scope.
     */
    virtual opt::optional<std::string> findAbsoluteDocumentUri(
            const opt::optional<std::string> resolutionScope,
            const opt::optional<std::string> documentUri)
    {
        if (resolutionScope) {
            if (documentUri) {
                if (internal::uri::isUriAbsolute(*documentUri)) {
                    return *documentUri;
                } else {
                    return internal::uri::resolveRelativeUri(
                            *resolutionScope, *documentUri);
                }
            } else {
                return *resolutionScope;
            }
        } else if (documentUri && internal::uri::isUriAbsolute(*documentUri)) {
            return *documentUri;
        } else {
            return opt::optional<std::string>();
        }
    }

    /**
     * @brief  Extract a JSON Reference string from a node
     *
     * @param  node    node to extract the JSON Reference from
     * @param  result  reference to string to set with the result
     *
     * @throws std::invalid_argument if node is an object containing a `$ref`
     *         property but with a value that cannot be interpreted as a string
     *
     * @return \c true if a JSON Reference was extracted; \c false otherwise
     */
    template<typename AdapterType>
    bool extractJsonReference(const AdapterType &node, std::string &result)
    {
        if (!node.isObject()) {
            return false;
        }

        const typename AdapterType::Object o = node.getObject();
        const typename AdapterType::Object::const_iterator itr = o.find("$ref");
        if (itr == o.end()) {
            return false;
        } else if (!itr->second.asString(result)) {
            throw std::invalid_argument(
                    "$ref property expected to contain string value.");
        }

        return true;
    }

    /**
     * Sanitise an optional JSON Pointer, trimming trailing slashes
     */
    std::string sanitiseJsonPointer(const opt::optional<std::string> input)
    {
        if (input) {
            // Trim trailing slash(es)
            std::string sanitised = *input;
            sanitised.erase(sanitised.find_last_not_of('/') + 1,
                    std::string::npos);

            return sanitised;
        }

        // If the JSON Pointer is not set, assume that the URI points to
        // the root of the document
        return "";
    }

    /**
     * @brief  Search the schema cache for a schema matching a given key
     *
     * If the key is not present in the query cache, a nullptr will be
     * returned, and the contents of the cache will remain unchanged. This is
     * in contrast to the behaviour of the std::map [] operator, which would
     * add the nullptr to the cache.
     *
     * @param  schemaCache  schema cache to query
     * @param  queryKey     key to search for
     *
     * @return shared pointer to Schema if found, nullptr otherwise
     */
    static const Subschema * querySchemaCache(SchemaCache &schemaCache,
            const std::string &queryKey)
    {
        const SchemaCache::iterator itr = schemaCache.find(queryKey);
        if (itr == schemaCache.end()) {
            return nullptr;
        }

        return itr->second;
    }

    /**
     * @brief  Add entries to the schema cache for a given list of keys
     *
     * @param  schemaCache   schema cache to update
     * @param  keysToCreate  list of keys to create entries for
     * @param  schema        shared pointer to schema that keys will map to
     *
     * @throws std::logic_error if any of the keys are already present in the
     *         schema cache. This behaviour is intended to help detect incorrect
     *         usage of the schema cache during development, and is not expected
     *         to occur otherwise, even for malformed schemas.
     */
    void updateSchemaCache(SchemaCache &schemaCache,
            const std::vector<std::string> &keysToCreate,
            const Subschema *schema)
    {
        for (const std::string &keyToCreate : keysToCreate) {
            const SchemaCache::value_type value(keyToCreate, schema);
            if (!schemaCache.insert(value).second) {
                throw std::logic_error(
                        "Key '" + keyToCreate + "' already in schema cache.");
            }
        }
    }

    /**
     * @brief  Recursive helper function for retrieving or creating schemas
     *
     * This function will be applied recursively until a concrete node is found.
     * A concrete node is a node that contains actual schema constraints rather
     * than a JSON Reference.
     *
     * This termination condition may be trigged by visiting the concrete node
     * at the end of a series of $ref nodes, or by finding a schema for one of
     * those $ref nodes in the schema cache. An entry will be added to the
     * schema cache for each node visited on the path to the concrete node.
     *
     * @param  rootSchema    The Schema instance, and root subschema, through
     *                       which other subschemas can be created and
     *                       modified
     * @param  rootNode      Reference to the node from which JSON References
     *                       will be resolved when they refer to the current
     *                       document
     * @param  node          Reference to the node to parse
     * @param  currentScope  URI for current resolution scope
     * @param  nodePath      JSON Pointer representing path to current node
     * @param  fetchDoc      Function to fetch remote JSON documents (optional)
     * @param  parentSchema  Optional pointer to the parent schema, used to
     *                       support required keyword in Draft 3
     * @param  ownName       Optional pointer to a node name, used to support
     *                       the 'required' keyword in Draft 3
     * @param  docCache      Cache of resolved and fetched remote documents
     * @param  schemaCache   Cache of populated schemas
     * @param  newCacheKeys  A list of keys that should be added to the cache
     *                       when recursion terminates
     */
    template<typename AdapterType>
    const Subschema * makeOrReuseSchema(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType &node,
        const opt::optional<std::string> currentScope,
        const std::string &nodePath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        const Subschema *parentSubschema,
        const std::string *ownName,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache,
        std::vector<std::string> &newCacheKeys)
    {
        std::string jsonRef;

        // Check for the first termination condition (found a non-$ref node)
        if (!extractJsonReference(node, jsonRef)) {

            // Construct a key that we can use to search the schema cache for
            // a schema corresponding to the current node
            const std::string schemaCacheKey =
                    currentScope ? (*currentScope + nodePath) : nodePath;

            // Retrieve an existing schema from the cache if possible
            const Subschema *cachedPtr =
                    querySchemaCache(schemaCache, schemaCacheKey);

            // Create a new schema otherwise
            const Subschema *subschema = cachedPtr ? cachedPtr :
                    rootSchema.createSubschema();

            // Add cache entries for keys belonging to any $ref nodes that were
            // visited before arriving at the current node
            updateSchemaCache(schemaCache, newCacheKeys, subschema);

            // Schema cache did not contain a pre-existing schema corresponding
            // to the current node, so the schema that was returned will need
            // to be populated
            if (!cachedPtr) {
                populateSchema(rootSchema, rootNode, node, *subschema,
                        currentScope, nodePath, fetchDoc, parentSubschema,
                        ownName, docCache, schemaCache);
            }

            return subschema;
        }

        // Returns a document URI if the reference points somewhere
        // other than the current document
        const opt::optional<std::string> documentUri =
                internal::json_reference::getJsonReferenceUri(jsonRef);

        // Extract JSON Pointer from JSON Reference, with any trailing
        // slashes removed so that keys in the schema cache end
        // consistently
        const std::string actualJsonPointer = sanitiseJsonPointer(
                internal::json_reference::getJsonReferencePointer(jsonRef));

        // Determine the actual document URI based on the resolution
        // scope. An absolute document URI will take precedence when
        // present, otherwise we need to resolve the URI relative to
        // the current resolution scope
        const opt::optional<std::string> actualDocumentUri =
                findAbsoluteDocumentUri(currentScope, documentUri);

        // Construct a key to search the schema cache for an existing schema
        const std::string queryKey = actualDocumentUri ?
                (*actualDocumentUri + actualJsonPointer) : actualJsonPointer;

        // Check for the second termination condition (found a $ref node that
        // already has an entry in the schema cache)
        const Subschema *cachedPtr = querySchemaCache(schemaCache, queryKey);
        if (cachedPtr) {
            updateSchemaCache(schemaCache, newCacheKeys, cachedPtr);
            return cachedPtr;
        }

        if (actualDocumentUri && (!currentScope || *actualDocumentUri != *currentScope)) {
            const typename FunctionPtrs<AdapterType>::DocumentType *newDoc = nullptr;

            // Have we seen this document before?
            typename DocumentCache<AdapterType>::Type::iterator docCacheItr =
                    docCache.find(*actualDocumentUri);
            if (docCacheItr == docCache.end()) {
                // Resolve reference against remote document
                if (!fetchDoc) {
                    throw std::runtime_error(
                            "Fetching of remote JSON References not enabled.");
                }

                // Returns a pointer to the remote document that was
                // retrieved, or null if retrieval failed. This class
                // will take ownership of the pointer, and call freeDoc
                // when it is no longer needed.
                newDoc = fetchDoc(*actualDocumentUri);

                // Can't proceed without the remote document
                if (!newDoc) {
                    throw std::runtime_error(
                            "Failed to fetch referenced schema document: " +
                            *actualDocumentUri);
                }

                typedef typename DocumentCache<AdapterType>::Type::value_type
                        DocCacheValueType;

                docCache.insert(DocCacheValueType(*actualDocumentUri, newDoc));

            } else {
                newDoc = docCacheItr->second;
            }

            const AdapterType newRootNode(*newDoc);

            // Find where we need to be in the document
            const AdapterType &referencedAdapter =
                    internal::json_pointer::resolveJsonPointer(newRootNode,
                            actualJsonPointer);

            newCacheKeys.push_back(queryKey);

            // Populate the schema, starting from the referenced node, with
            // nested JSON References resolved relative to the new root node
            return makeOrReuseSchema(rootSchema, newRootNode, referencedAdapter,
                    currentScope, actualJsonPointer, fetchDoc, parentSubschema,
                    ownName, docCache, schemaCache, newCacheKeys);

        }

        // JSON References in nested schema will be resolved relative to the
        // current document
        const AdapterType &referencedAdapter =
                internal::json_pointer::resolveJsonPointer(
                        rootNode, actualJsonPointer);

        newCacheKeys.push_back(queryKey);

        // Populate the schema, starting from the referenced node, with
        // nested JSON References resolved relative to the new root node
        return makeOrReuseSchema(rootSchema, rootNode, referencedAdapter,
                currentScope, actualJsonPointer, fetchDoc, parentSubschema,
                ownName, docCache, schemaCache, newCacheKeys);
    }

    /**
     * @brief  Return pointer for the schema corresponding to a given node
     *
     * This function makes use of a schema cache, so that if the path to the
     * current node is the same as one that has already been parsed and
     * populated, a pointer to the existing Subschema will be returned.
     *
     * Should a series of $ref, or reference, nodes be resolved before reaching
     * a concrete node, an entry will be added to the schema cache for each of
     * the nodes in that path.
     *
     * @param  rootSchema    The Schema instance, and root subschema, through
     *                       which other subschemas can be created and
     *                       modified
     * @param  rootNode      Reference to the node from which JSON References
     *                       will be resolved when they refer to the current
     *                       document
     * @param  node          Reference to the node to parse
     * @param  currentScope  URI for current resolution scope
     * @param  nodePath      JSON Pointer representing path to current node
     * @param  fetchDoc      Function to fetch remote JSON documents (optional)
     * @param  parentSchema  Optional pointer to the parent schema, used to
     *                       support required keyword in Draft 3
     * @param  ownName       Optional pointer to a node name, used to support
     *                       the 'required' keyword in Draft 3
     * @param  docCache      Cache of resolved and fetched remote documents
     * @param  schemaCache   Cache of populated schemas
     */
    template<typename AdapterType>
    const Subschema * makeOrReuseSchema(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType &node,
        const opt::optional<std::string> currentScope,
        const std::string &nodePath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        const Subschema *parentSubschema,
        const std::string *ownName,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache)
    {
        std::vector<std::string> schemaCacheKeysToCreate;

        return makeOrReuseSchema(rootSchema, rootNode, node, currentScope,
                nodePath, fetchDoc, parentSubschema, ownName, docCache,
                schemaCache, schemaCacheKeysToCreate);
    }

    /**
     * @brief  Populate a Schema object from JSON Schema document
     *
     * When processing Draft 3 schemas, the parentSubschema and ownName pointers
     * should be set in contexts where a 'required' constraint would be valid.
     * These are used to add a RequiredConstraint object to the Schema that
     * contains the required property.
     *
     * @param  rootSchema       The Schema instance, and root subschema, through
     *                          which other subschemas can be created and
     *                          modified
     * @param  rootNode         Reference to the node from which JSON References
     *                          will be resolved when they refer to the current
     *                          document
     * @param  node             Reference to node to parse
     * @param  schema           Reference to Schema to populate
     * @param  currentScope     URI for current resolution scope
     * @param  nodePath         JSON Pointer representing path to current node
     * @param  fetchDoc         Optional function to fetch remote JSON documents
     * @param  parentSubschema  Optional pointer to the parent schema, used to
     *                          support required keyword in Draft 3
     * @param  ownName          Optional pointer to a node name, used to support
     *                          the 'required' keyword in Draft 3
     * @param  docCache         Cache of resolved and fetched remote documents
     * @param  schemaCache      Cache of populated schemas
     */
    template<typename AdapterType>
    void populateSchema(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType &node,
        const Subschema &subschema,
        const opt::optional<std::string> currentScope,
        const std::string &nodePath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        const Subschema *parentSubschema,
        const std::string *ownName,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache)
    {
        static_assert((std::is_convertible<AdapterType,
            const valijson::adapters::Adapter &>::value),
            "SchemaParser::populateSchema must be invoked with an "
            "appropriate Adapter implementation");

        if (!node.isObject()) {
            if (version == kDraft7 && node.maybeBool()) {
                // Boolean schema
                if (!node.asBool()) {
                    rootSchema.setAlwaysInvalid(&subschema, true);
                }
                return;
            } else {
                std::string s;
                s += "Expected node at ";
                s += nodePath;
                if (version == kDraft7) {
                    s += " to contain schema object or boolean value; actual node type is: ";
                } else {
                    s += " to contain schema object; actual node type is: ";
                }
                s += internal::nodeTypeAsString(node);
                throw std::runtime_error(s);
            }
        }

        const typename AdapterType::Object object = node.asObject();
        typename AdapterType::Object::const_iterator itr(object.end());

        // Check for 'id' attribute and update current scope
        opt::optional<std::string> updatedScope;
        if ((itr = object.find("id")) != object.end() &&
                itr->second.maybeString()) {
            const std::string id = itr->second.asString();
            rootSchema.setSubschemaId(&subschema, itr->second.asString());
            if (!currentScope || internal::uri::isUriAbsolute(id)) {
                updatedScope = id;
            } else {
                updatedScope = internal::uri::resolveRelativeUri(
                        *currentScope, id);
            }
        } else {
            updatedScope = currentScope;
        }

        if ((itr = object.find("allOf")) != object.end()) {
            rootSchema.addConstraintToSubschema(
                    makeAllOfConstraint(rootSchema, rootNode, itr->second,
                            updatedScope, nodePath + "/allOf", fetchDoc,
                            docCache, schemaCache),
                    &subschema);
        }

        if ((itr = object.find("anyOf")) != object.end()) {
            rootSchema.addConstraintToSubschema(
                    makeAnyOfConstraint(rootSchema, rootNode, itr->second,
                            updatedScope, nodePath + "/anyOf", fetchDoc,
                            docCache, schemaCache),
                    &subschema);
        }

        if ((itr = object.find("const")) != object.end()) {
            rootSchema.addConstraintToSubschema(makeConstConstraint(itr->second), &subschema);
        }

        if ((itr = object.find("contains")) != object.end()) {
            rootSchema.addConstraintToSubschema(
                    makeContainsConstraint(rootSchema, rootNode, itr->second,
                            updatedScope, nodePath + "/contains", fetchDoc,
                            docCache, schemaCache), &subschema);
        }

        if ((itr = object.find("dependencies")) != object.end()) {
            rootSchema.addConstraintToSubschema(
                    makeDependenciesConstraint(rootSchema, rootNode,
                            itr->second, updatedScope,
                            nodePath + "/dependencies", fetchDoc, docCache,
                            schemaCache),
                    &subschema);
        }

        if ((itr = object.find("description")) != object.end()) {
            if (itr->second.maybeString()) {
                rootSchema.setSubschemaDescription(&subschema,
                        itr->second.asString());
            } else {
                throw std::runtime_error(
                        "'description' attribute should have a string value");
            }
        }

        if ((itr = object.find("divisibleBy")) != object.end()) {
            if (version == kDraft3) {
                if (itr->second.maybeInteger()) {
                    rootSchema.addConstraintToSubschema(
                            makeMultipleOfIntConstraint(itr->second),
                            &subschema);
                } else if (itr->second.maybeDouble()) {
                    rootSchema.addConstraintToSubschema(
                            makeMultipleOfDoubleConstraint(itr->second),
                            &subschema);
                } else {
                    throw std::runtime_error("Expected an numeric value for "
                            " 'divisibleBy' constraint.");
                }
            } else {
                throw std::runtime_error(
                        "'divisibleBy' constraint not valid after draft 3");
            }
        }

        if ((itr = object.find("enum")) != object.end()) {
            rootSchema.addConstraintToSubschema(makeEnumConstraint(itr->second), &subschema);
        }

        {
            const typename AdapterType::Object::const_iterator itemsItr =
                    object.find("items");

            if (object.end() != itemsItr) {
                if (!itemsItr->second.isArray()) {
                    rootSchema.addConstraintToSubschema(
                            makeSingularItemsConstraint(rootSchema, rootNode,
                                    itemsItr->second, updatedScope,
                                    nodePath + "/items", fetchDoc, docCache,
                                    schemaCache),
                            &subschema);

                } else {
                    const typename AdapterType::Object::const_iterator
                            additionalItemsItr = object.find("additionalItems");
                    rootSchema.addConstraintToSubschema(
                            makeLinearItemsConstraint(rootSchema, rootNode,
                                    itemsItr != object.end() ?
                                            &itemsItr->second : nullptr,
                                    additionalItemsItr != object.end() ?
                                            &additionalItemsItr->second : nullptr,
                                    updatedScope, nodePath + "/items",
                                    nodePath + "/additionalItems", fetchDoc,
                                    docCache, schemaCache),
                            &subschema);
                }
            }
        }

        {
            const typename AdapterType::Object::const_iterator ifItr = object.find("if");
            const typename AdapterType::Object::const_iterator thenItr = object.find("then");
            const typename AdapterType::Object::const_iterator elseItr = object.find("else");

            if (object.end() != ifItr) {
                if (version == kDraft7) {
                    rootSchema.addConstraintToSubschema(
                          makeConditionalConstraint(rootSchema, rootNode,
                                ifItr->second,
                                thenItr == object.end() ? nullptr : &thenItr->second,
                                elseItr == object.end() ? nullptr : &elseItr->second,
                                updatedScope, nodePath, fetchDoc, docCache, schemaCache),
                          &subschema);
                } else {
                    throw std::runtime_error("Not supported");
                }
            }
        }

        if (version == kDraft7) {
            if ((itr = object.find("exclusiveMaximum")) != object.end()) {
                rootSchema.addConstraintToSubschema(
                    makeMaximumConstraintExclusive(itr->second),
                    &subschema);
            }

            if ((itr = object.find("maximum")) != object.end()) {
                rootSchema.addConstraintToSubschema(
                    makeMaximumConstraint<AdapterType>(itr->second, nullptr),
                    &subschema);
            }
        } else if ((itr = object.find("maximum")) != object.end()) {
            typename AdapterType::Object::const_iterator exclusiveMaximumItr =
                    object.find("exclusiveMaximum");
            if (exclusiveMaximumItr == object.end()) {
                rootSchema.addConstraintToSubschema(
                        makeMaximumConstraint<AdapterType>(itr->second, nullptr),
                        &subschema);
            } else {
                rootSchema.addConstraintToSubschema(
                        makeMaximumConstraint(itr->second,
                                &exclusiveMaximumItr->second),
                        &subschema);
            }
        } else if (object.find("exclusiveMaximum") != object.end()) {
            throw std::runtime_error(
                    "'exclusiveMaximum' constraint only valid if a 'maximum' "
                    "constraint is also present");
        }

        if ((itr = object.find("maxItems")) != object.end()) {
            rootSchema.addConstraintToSubschema(
                    makeMaxItemsConstraint(itr->second), &subschema);
        }

        if ((itr = object.find("maxLength")) != object.end()) {
            rootSchema.addConstraintToSubschema(
                    makeMaxLengthConstraint(itr->second), &subschema);
        }

        if ((itr = object.find("maxProperties")) != object.end()) {
            rootSchema.addConstraintToSubschema(
                    makeMaxPropertiesConstraint(itr->second), &subschema);
        }

        if (version == kDraft7) {
            if ((itr = object.find("exclusiveMinimum")) != object.end()) {
                rootSchema.addConstraintToSubschema(
                        makeMinimumConstraintExclusive(itr->second),
                        &subschema);
            }

            if ((itr = object.find("minimum")) != object.end()) {
                rootSchema.addConstraintToSubschema(
                        makeMinimumConstraint<AdapterType>(itr->second, nullptr),
                        &subschema);
            }
        } else if ((itr = object.find("minimum")) != object.end()) {
            typename AdapterType::Object::const_iterator exclusiveMinimumItr =
                    object.find("exclusiveMinimum");
            if (exclusiveMinimumItr == object.end()) {
                rootSchema.addConstraintToSubschema(
                        makeMinimumConstraint<AdapterType>(itr->second, nullptr),
                        &subschema);
            } else {
                rootSchema.addConstraintToSubschema(
                        makeMinimumConstraint<AdapterType>(
                                itr->second,
                                &exclusiveMinimumItr->second),
                        &subschema);
            }
        } else if (object.find("exclusiveMinimum") != object.end()) {
            throw std::runtime_error(
                    "'exclusiveMinimum' constraint only valid if a 'minimum' "
                    "constraint is also present");
        }

        if ((itr = object.find("minItems")) != object.end()) {
            rootSchema.addConstraintToSubschema(
                    makeMinItemsConstraint(itr->second), &subschema);
        }

        if ((itr = object.find("minLength")) != object.end()) {
            rootSchema.addConstraintToSubschema(
                    makeMinLengthConstraint(itr->second), &subschema);
        }

        if ((itr = object.find("minProperties")) != object.end()) {
            rootSchema.addConstraintToSubschema(
                    makeMinPropertiesConstraint(itr->second), &subschema);
        }

        if ((itr = object.find("multipleOf")) != object.end()) {
            if (version == kDraft3) {
                throw std::runtime_error(
                        "'multipleOf' constraint not available in draft 3");
            } else if (itr->second.maybeInteger()) {
                rootSchema.addConstraintToSubschema(
                        makeMultipleOfIntConstraint(itr->second),
                        &subschema);
            } else if (itr->second.maybeDouble()) {
                rootSchema.addConstraintToSubschema(
                        makeMultipleOfDoubleConstraint(itr->second),
                        &subschema);
            } else {
                throw std::runtime_error("Expected an numeric value for "
                        " 'divisibleBy' constraint.");
            }
        }

        if ((itr = object.find("not")) != object.end()) {
            rootSchema.addConstraintToSubschema(
                    makeNotConstraint(rootSchema, rootNode, itr->second,
                            updatedScope, nodePath + "/not", fetchDoc, docCache,
                            schemaCache),
                    &subschema);
        }

        if ((itr = object.find("oneOf")) != object.end()) {
            rootSchema.addConstraintToSubschema(
                    makeOneOfConstraint(rootSchema, rootNode, itr->second,
                            updatedScope, nodePath + "/oneOf", fetchDoc,
                            docCache, schemaCache),
                    &subschema);
        }

        if ((itr = object.find("pattern")) != object.end()) {
            rootSchema.addConstraintToSubschema(
                    makePatternConstraint(itr->second), &subschema);
        }

        {
            // Check for schema keywords that require the creation of a
            // PropertiesConstraint instance.
            const typename AdapterType::Object::const_iterator
                propertiesItr = object.find("properties"),
                patternPropertiesItr = object.find("patternProperties"),
                additionalPropertiesItr = object.find("additionalProperties");
            if (object.end() != propertiesItr ||
                object.end() != patternPropertiesItr ||
                object.end() != additionalPropertiesItr) {
                rootSchema.addConstraintToSubschema(
                        makePropertiesConstraint(rootSchema, rootNode,
                                propertiesItr != object.end() ?
                                        &propertiesItr->second : nullptr,
                                patternPropertiesItr != object.end() ?
                                        &patternPropertiesItr->second : nullptr,
                                additionalPropertiesItr != object.end() ?
                                        &additionalPropertiesItr->second : nullptr,
                                updatedScope, nodePath + "/properties",
                                nodePath + "/patternProperties",
                                nodePath + "/additionalProperties",
                                fetchDoc, &subschema, docCache, schemaCache),
                        &subschema);
            }
        }

        if ((itr = object.find("propertyNames")) != object.end()) {
            if (version == kDraft7) {
                rootSchema.addConstraintToSubschema(
                      makePropertyNamesConstraint(rootSchema, rootNode, itr->second, updatedScope,
                              nodePath, fetchDoc, docCache, schemaCache),
                      &subschema);
            } else {
                throw std::runtime_error("Not supported");
            }
        }

        if ((itr = object.find("required")) != object.end()) {
            if (version == kDraft3) {
                if (parentSubschema && ownName) {
                    opt::optional<constraints::RequiredConstraint>
                            constraint = makeRequiredConstraintForSelf(
                                    itr->second, *ownName);
                    if (constraint) {
                        rootSchema.addConstraintToSubschema(*constraint,
                                parentSubschema);
                    }
                } else {
                    throw std::runtime_error(
                            "'required' constraint not valid here");
                }
            } else {
                rootSchema.addConstraintToSubschema(
                        makeRequiredConstraint(itr->second), &subschema);
            }
        }

        if ((itr = object.find("title")) != object.end()) {
            if (itr->second.maybeString()) {
                rootSchema.setSubschemaTitle(&subschema,
                        itr->second.asString());
            } else {
                throw std::runtime_error(
                        "'title' attribute should have a string value");
            }
        }

        if ((itr = object.find("type")) != object.end()) {
            rootSchema.addConstraintToSubschema(
                    makeTypeConstraint(rootSchema, rootNode, itr->second,
                            updatedScope, nodePath + "/type", fetchDoc,
                            docCache, schemaCache),
                    &subschema);
        }

        if ((itr = object.find("uniqueItems")) != object.end()) {
            opt::optional<constraints::UniqueItemsConstraint> constraint =
                    makeUniqueItemsConstraint(itr->second);
            if (constraint) {
                rootSchema.addConstraintToSubschema(*constraint, &subschema);
            }
        }

        for (ConstraintBuilders::const_iterator
                builderItr  = constraintBuilders.begin();
                builderItr != constraintBuilders.end(); ++builderItr) {
            if ((itr = object.find(builderItr->first)) != object.end()) {
                constraints::Constraint *constraint = nullptr;
                try {
                    constraint = builderItr->second->make(itr->second);
                    rootSchema.addConstraintToSubschema(*constraint,
                            &subschema);
                    delete constraint;
                } catch (...) {
                    delete constraint;
                    throw;
                }
            }
        }
    }

    /**
     * @brief  Resolves a chain of JSON References before populating a schema
     *
     * This helper function is used directly by the publicly visible
     * populateSchema function. It ensures that the node being parsed is a
     * concrete node, and not a JSON Reference. This function will call itself
     * recursively to resolve references until a concrete node is found.
     *
     * @param  rootSchema    The Schema instance, and root subschema, through
     *                       which other subschemas can be created and modified
     * @param  rootNode      Reference to the node from which JSON References
     *                       will be resolved when they refer to the current
     *                       document
     * @param  node          Reference to node to parse
     * @param  subschema     Reference to Schema to populate
     * @param  currentScope  URI for current resolution scope
     * @param  nodePath      JSON Pointer representing path to current node
     * @param  fetchDoc      Function to fetch remote JSON documents (optional)
     * @param  parentSchema  Optional pointer to the parent schema, used to
     *                       support required keyword in Draft 3
     * @param  ownName       Optional pointer to a node name, used to support
     *                       the 'required' keyword in Draft 3
     * @param  docCache      Cache of resolved and fetched remote documents
     * @param  schemaCache   Cache of populated schemas
     */
    template<typename AdapterType>
    void resolveThenPopulateSchema(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType &node,
        const Subschema &subschema,
        const opt::optional<std::string> currentScope,
        const std::string &nodePath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        const Subschema *parentSchema,
        const std::string *ownName,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache)
    {
        std::string jsonRef;
        if (!extractJsonReference(node, jsonRef)) {
            populateSchema(rootSchema, rootNode, node, subschema, currentScope,
                    nodePath, fetchDoc, parentSchema, ownName, docCache,
                    schemaCache);
            return;
        }

        // Returns a document URI if the reference points somewhere
        // other than the current document
        const opt::optional<std::string> documentUri =
                internal::json_reference::getJsonReferenceUri(jsonRef);

        // Extract JSON Pointer from JSON Reference
        const std::string actualJsonPointer = sanitiseJsonPointer(
                internal::json_reference::getJsonReferencePointer(jsonRef));

        if (documentUri && internal::uri::isUriAbsolute(*documentUri)) {
            // Resolve reference against remote document
            if (!fetchDoc) {
                throw std::runtime_error(
                        "Fetching of remote JSON References not enabled.");
            }

            const typename DocumentCache<AdapterType>::DocumentType *newDoc =
                    fetchDoc(*documentUri);

            // Can't proceed without the remote document
            if (!newDoc) {
                throw std::runtime_error(
                        "Failed to fetch referenced schema document: " +
                        *documentUri);
            }

            // Add to document cache
            typedef typename DocumentCache<AdapterType>::Type::value_type
                    DocCacheValueType;

            docCache.insert(DocCacheValueType(*documentUri, newDoc));

            const AdapterType newRootNode(*newDoc);

            const AdapterType &referencedAdapter =
                internal::json_pointer::resolveJsonPointer(
                        newRootNode, actualJsonPointer);

            // TODO: Need to detect degenerate circular references
            resolveThenPopulateSchema(rootSchema, newRootNode,
                    referencedAdapter, subschema, opt::optional<std::string>(),
                    actualJsonPointer, fetchDoc, parentSchema, ownName,
                    docCache, schemaCache);

        } else {
            const AdapterType &referencedAdapter =
                internal::json_pointer::resolveJsonPointer(
                        rootNode, actualJsonPointer);

            // TODO: Need to detect degenerate circular references
            resolveThenPopulateSchema(rootSchema, rootNode, referencedAdapter,
                    subschema, opt::optional<std::string>(),
                    actualJsonPointer, fetchDoc,
                    parentSchema, ownName, docCache, schemaCache);
        }
    }

    /**
     * @brief   Make a new AllOfConstraint object
     *
     * @param   rootSchema    The Schema instance, and root subschema, through
     *                        which other subschemas can be created and modified
     * @param   rootNode      Reference to the node from which JSON References
     *                        will be resolved when they refer to the current
     *                        document; used for recursive parsing of schemas
     * @param   node          JSON node containing an array of child schemas
     * @param   currentScope  URI for current resolution scope
     * @param   nodePath      JSON Pointer representing path to current node
     * @param   fetchDoc      Function to fetch remote JSON documents (optional)
     * @param   docCache      Cache of resolved and fetched remote documents
     * @param   schemaCache   Cache of populated schemas
     *
     * @return  pointer to a new AllOfConstraint object that belongs to the
     *          caller
     */
    template<typename AdapterType>
    constraints::AllOfConstraint makeAllOfConstraint(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType &node,
        const opt::optional<std::string> currentScope,
        const std::string &nodePath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache)
    {
        if (!node.maybeArray()) {
            throw std::runtime_error(
                    "Expected array value for 'allOf' constraint.");
        }

        constraints::AllOfConstraint constraint;

        int index = 0;
        for (const AdapterType schemaNode : node.asArray()) {
            if (schemaNode.maybeObject() || (version == kDraft7 && schemaNode.isBool())) {
                const std::string childPath = nodePath + "/" + std::to_string(index);
                const Subschema *subschema = makeOrReuseSchema<AdapterType>(
                        rootSchema, rootNode, schemaNode, currentScope,
                        childPath, fetchDoc, nullptr, nullptr, docCache, schemaCache);
                constraint.addSubschema(subschema);
                index++;
            } else {
                throw std::runtime_error(
                        "Expected element to be a valid schema in 'allOf' constraint.");
            }
        }

        return constraint;
    }

    /**
     * @brief   Make a new AnyOfConstraint object
     *
     * @param   rootSchema    The Schema instance, and root subschema, through
     *                        which other subschemas can be created and modified
     * @param   rootNode      Reference to the node from which JSON References
     *                        will be resolved when they refer to the current
     *                        document; used for recursive parsing of schemas
     * @param   node          JSON node containing an array of child schemas
     * @param   currentScope  URI for current resolution scope
     * @param   nodePath      JSON Pointer representing path to current node
     * @param   fetchDoc      Function to fetch remote JSON documents (optional)
     * @param   docCache      Cache of resolved and fetched remote documents
     * @param   schemaCache   Cache of populated schemas
     *
     * @return  pointer to a new AnyOfConstraint object that belongs to the
     *          caller
     */
    template<typename AdapterType>
    constraints::AnyOfConstraint makeAnyOfConstraint(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType &node,
        const opt::optional<std::string> currentScope,
        const std::string &nodePath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache)
    {
        if (!node.maybeArray()) {
            throw std::runtime_error(
                    "Expected array value for 'anyOf' constraint.");
        }

        constraints::AnyOfConstraint constraint;

        int index = 0;
        for (const AdapterType schemaNode : node.asArray()) {
            if (schemaNode.maybeObject() || (version == kDraft7 && schemaNode.isBool())) {
                const std::string childPath = nodePath + "/" + std::to_string(index);
                const Subschema *subschema = makeOrReuseSchema<AdapterType>(
                        rootSchema, rootNode, schemaNode, currentScope,
                        childPath, fetchDoc, nullptr, nullptr, docCache, schemaCache);
                constraint.addSubschema(subschema);
                index++;
            } else {
                throw std::runtime_error(
                        "Expected array element to be a valid schema in 'anyOf' constraint.");
            }
        }

        return constraint;
    }

    /**
     * @brief   Make a new ConditionalConstraint object.
     *
     * @param   rootSchema           The Schema instance, and root subschema,
     *                               through which other subschemas can be
     *                               created and modified
     * @param   rootNode             Reference to the node from which JSON
     *                               References will be resolved when they refer
     *                               to the current document; used for recursive
     *                               parsing of schemas
     * @param   ifNode               Schema that will be used to evaluate the
     *                               conditional.
     * @param   thenNode             Optional pointer to a JSON node containing
     *                               a schema that will be used when the conditional
     *                               evaluates to true.
     * @param   elseNode             Optional pointer to a JSON node containing
     *                               a schema that will be used when the conditional
     *                               evaluates to false.
     * @param   currentScope         URI for current resolution scope
     * @param   containsPath         JSON Pointer representing the path to
     *                               the 'contains' node
     * @param   fetchDoc             Function to fetch remote JSON documents
     *                               (optional)
     * @param   docCache             Cache of resolved and fetched remote
     *                               documents
     * @param   schemaCache          Cache of populated schemas
     *
     * @return  pointer to a new ContainsConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::ConditionalConstraint makeConditionalConstraint(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType &ifNode,
        const AdapterType *thenNode,
        const AdapterType *elseNode,
        const opt::optional<std::string> currentScope,
        const std::string &nodePath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache)
    {
        constraints::ConditionalConstraint constraint;

        const Subschema *ifSubschema = makeOrReuseSchema<AdapterType>(
                rootSchema, rootNode, ifNode, currentScope,
                nodePath, fetchDoc, nullptr, nullptr, docCache,
                schemaCache);
        constraint.setIfSubschema(ifSubschema);

        if (thenNode) {
            const Subschema *thenSubschema = makeOrReuseSchema<AdapterType>(
                    rootSchema, rootNode, *thenNode, currentScope, nodePath, fetchDoc, nullptr,
                    nullptr, docCache, schemaCache);
            constraint.setThenSubschema(thenSubschema);
        }

        if (elseNode) {
            const Subschema *elseSubschema = makeOrReuseSchema<AdapterType>(
                    rootSchema, rootNode, *elseNode, currentScope, nodePath, fetchDoc, nullptr,
                    nullptr, docCache, schemaCache);
            constraint.setElseSubschema(elseSubschema);
        }

        return constraint;
    }

    /**
     * @brief   Make a new ConstConstraint object.
     *
     * @param   node  JSON node containing an arbitrary value
     *
     * @return  pointer to a new MinimumConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::ConstConstraint makeConstConstraint(const AdapterType &node)
    {
        constraints::ConstConstraint constraint;
        constraint.setValue(node);
        return constraint;
    }

    /**
     * @brief   Make a new ContainsConstraint object.
     *
     * @param   rootSchema           The Schema instance, and root subschema,
     *                               through which other subschemas can be
     *                               created and modified
     * @param   rootNode             Reference to the node from which JSON
     *                               References will be resolved when they refer
     *                               to the current document; used for recursive
     *                               parsing of schemas
     * @param   contains             Optional pointer to a JSON node containing
     *                               an object mapping property names to
     *                               schemas.
     * @param   currentScope         URI for current resolution scope
     * @param   containsPath         JSON Pointer representing the path to
     *                               the 'contains' node
     * @param   fetchDoc             Function to fetch remote JSON documents
     *                               (optional)
     * @param   docCache             Cache of resolved and fetched remote
     *                               documents
     * @param   schemaCache          Cache of populated schemas
     *
     * @return  pointer to a new ContainsConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::ContainsConstraint makeContainsConstraint(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType &contains,
        const opt::optional<std::string> currentScope,
        const std::string &containsPath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache)
    {
        constraints::ContainsConstraint constraint;

        if (contains.isObject() || (version == kDraft7 && contains.maybeBool())) {
            const Subschema *subschema = makeOrReuseSchema<AdapterType>(
                    rootSchema, rootNode, contains, currentScope, containsPath,
                    fetchDoc, nullptr, nullptr, docCache, schemaCache);
            constraint.setSubschema(subschema);

        } else if (contains.maybeObject()) {
            // If a loosely-typed Adapter type is being used, then we'll
            // assume that an empty schema has been provided.
            constraint.setSubschema(rootSchema.emptySubschema());

        } else {
            // All other formats will result in an exception being thrown.
            throw std::runtime_error(
                    "Expected valid schema for 'contains' constraint.");
        }

        return constraint;
    }

    /**
     * @brief   Make a new DependenciesConstraint object
     *
     * The dependencies for a property can be defined several ways. When parsing
     * a Draft 4 schema, the following can be used:
     *  - an array that lists the name of each property that must be present
     *    if the dependent property is present
     *  - an object that specifies a schema which must be satisfied if the
     *    dependent property is present
     *
     * When parsing a Draft 3 schema, in addition to the formats above, the
     * following format can be used:
     *  - a string that names a single property that must be present if the
     *    dependent property is presnet
     *
     * Multiple methods can be used in the same dependency constraint.
     *
     * If the format of any part of the the dependency node does not match one
     * of these formats, an exception will be thrown.
     *
     * @param   rootSchema    The Schema instance, and root subschema, through
     *                        which other subschemas can be created and modified
     * @param   rootNode      Reference to the node from which JSON References
     *                        will be resolved when they refer to the current
     *                        document; used for recursive parsing of schemas
     * @param   node          JSON node containing an object that defines a
     *                        mapping of properties to their dependencies.
     * @param   currentScope  URI for current resolution scope
     * @param   nodePath      JSON Pointer representing path to current node
     * @param   fetchDoc      Function to fetch remote JSON documents (optional)
     * @param   docCache      Cache of resolved and fetched remote documents
     * @param   schemaCache   Cache of populated schemas
     *
     * @return  pointer to a new DependencyConstraint that belongs to the
     *          caller
     */
    template<typename AdapterType>
    constraints::DependenciesConstraint makeDependenciesConstraint(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType &node,
        const opt::optional<std::string> currentScope,
        const std::string &nodePath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache)
    {
        if (!node.maybeObject()) {
            throw std::runtime_error("Expected valid subschema for 'dependencies' constraint.");
        }

        constraints::DependenciesConstraint dependenciesConstraint;

        // Process each of the dependency mappings defined by the object
        for (const typename AdapterType::ObjectMember member : node.asObject()) {

            // First, we attempt to parse the value of the dependency mapping
            // as an array of strings. If the Adapter type does not support
            // strict types, then an empty string or empty object will be cast
            // to an array, and the resulting dependency list will be empty.
            // This is equivalent to using an empty object, but does mean that
            // if the user provides an actual string then this error will not
            // be detected.
            if (member.second.maybeArray()) {
                // Parse an array of dependency names
                std::vector<std::string> dependentPropertyNames;
                for (const AdapterType dependencyName : member.second.asArray()) {
                    if (dependencyName.maybeString()) {
                        dependentPropertyNames.push_back(dependencyName.getString());
                    } else {
                        throw std::runtime_error("Expected string value in dependency list of property '" +
                            member.first + "' in 'dependencies' constraint.");
                    }
                }

                dependenciesConstraint.addPropertyDependencies(member.first,
                        dependentPropertyNames);

            // If the value of dependency mapping could not be processed as an
            // array, we'll try to process it as an object instead. Note that
            // strict type comparison is used here, since we've already
            // exercised the flexibility by loosely-typed Adapter types. If the
            // value of the dependency mapping is an object, then we'll try to
            // process it as a dependent schema.
            } else if (member.second.isObject() || (version == kDraft7 && member.second.maybeBool())) {
                // Parse dependent subschema
                const Subschema *childSubschema =
                        makeOrReuseSchema<AdapterType>(rootSchema, rootNode,
                                member.second, currentScope, nodePath, fetchDoc,
                                nullptr, nullptr, docCache, schemaCache);
                dependenciesConstraint.addSchemaDependency(member.first,
                        childSubschema);

            // If we're supposed to be parsing a Draft3 schema, then the value
            // of the dependency mapping can also be a string containing the
            // name of a single dependency.
            } else if (version == kDraft3 && member.second.isString()) {
                dependenciesConstraint.addPropertyDependency(member.first,
                        member.second.getString());

            // All other types result in an exception being thrown.
            } else {
                throw std::runtime_error("Invalid dependencies definition.");
            }
        }

        return dependenciesConstraint;
    }

    /**
     * @brief   Make a new EnumConstraint object.
     *
     * @param   node  JSON node containing an array of values permitted by the
     *                constraint.
     *
     * @return  pointer to a new EnumConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::EnumConstraint makeEnumConstraint(
        const AdapterType &node)
    {
        // Make a copy of each value in the enum array
        constraints::EnumConstraint constraint;
        for (const AdapterType value : node.getArray()) {
            constraint.addValue(value);
        }

        /// @todo This will make another copy of the values while constructing
        /// the EnumConstraint. Move semantics in C++11 should make it possible
        /// to avoid these copies without complicating the implementation of the
        /// EnumConstraint class.
        return constraint;
    }

    /**
     * @brief   Make a new ItemsConstraint object.
     *
     * @param   rootSchema           The Schema instance, and root subschema,
     *                               through which other subschemas can be
     *                               created and modified
     * @param   rootNode             Reference to the node from which JSON
     *                               References will be resolved when they refer
     *                               to the current document; used for recursive
     *                               parsing of schemas
     * @param   items                Optional pointer to a JSON node containing
     *                               an object mapping property names to
     *                               schemas.
     * @param   additionalItems      Optional pointer to a JSON node containing
     *                               an additional properties schema or a
     *                               boolean value.
     * @param   currentScope         URI for current resolution scope
     * @param   itemsPath            JSON Pointer representing the path to
     *                               the 'items' node
     * @param   additionalItemsPath  JSON Pointer representing the path to
     *                               the 'additionalItems' node
     * @param   fetchDoc             Function to fetch remote JSON documents
     *                               (optional)
     * @param   docCache             Cache of resolved and fetched remote
     *                               documents
     * @param   schemaCache          Cache of populated schemas
     *
     * @return  pointer to a new ItemsConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::LinearItemsConstraint makeLinearItemsConstraint(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType *items,
        const AdapterType *additionalItems,
        const opt::optional<std::string> currentScope,
        const std::string &itemsPath,
        const std::string &additionalItemsPath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache)
    {
        constraints::LinearItemsConstraint constraint;

        // Construct a Schema object for the the additionalItems constraint,
        // if the additionalItems property is present
        if (additionalItems) {
            if (additionalItems->maybeBool()) {
                // If the value of the additionalItems property is a boolean
                // and is set to true, then additional array items do not need
                // to satisfy any constraints.
                if (additionalItems->asBool()) {
                    constraint.setAdditionalItemsSubschema(
                            rootSchema.emptySubschema());
                }
            } else if (additionalItems->maybeObject()) {
                // If the value of the additionalItems property is an object,
                // then it should be parsed into a Schema object, which will be
                // used to validate additional array items.
                const Subschema *subschema = makeOrReuseSchema<AdapterType>(
                        rootSchema, rootNode, *additionalItems, currentScope,
                        additionalItemsPath, fetchDoc, nullptr, nullptr, docCache,
                        schemaCache);
                constraint.setAdditionalItemsSubschema(subschema);
            } else {
                // Any other format for the additionalItems property will result
                // in an exception being thrown.
                throw std::runtime_error(
                        "Expected bool or object value for 'additionalItems'");
            }
        } else {
            // The default value for the additionalItems property is an empty
            // object, which means that additional array items do not need to
            // satisfy any constraints.
            constraint.setAdditionalItemsSubschema(rootSchema.emptySubschema());
        }

        // Construct a Schema object for each item in the items array.
        // If the items constraint is not provided, then array items
        // will be validated against the additionalItems schema.
        if (items) {
            if (items->isArray()) {
                // If the items constraint contains an array, then it should
                // contain a list of child schemas which will be used to
                // validate the values at the corresponding indexes in a target
                // array.
                int index = 0;
                for (const AdapterType v : items->getArray()) {
                    const std::string childPath = itemsPath + "/" +
                            std::to_string(index);
                    const Subschema *subschema = makeOrReuseSchema<AdapterType>(
                            rootSchema, rootNode, v, currentScope, childPath,
                            fetchDoc, nullptr, nullptr, docCache, schemaCache);
                    constraint.addItemSubschema(subschema);
                    index++;
                }
            } else {
                throw std::runtime_error(
                        "Expected array value for non-singular 'items' "
                        "constraint.");
            }
        }

        return constraint;
    }

    /**
     * @brief   Make a new ItemsConstraint object.
     *
     * @param   rootSchema           The Schema instance, and root subschema,
     *                               through which other subschemas can be
     *                               created and modified
     * @param   rootNode             Reference to the node from which JSON
     *                               References will be resolved when they refer
     *                               to the current document; used for recursive
     *                               parsing of schemas
     * @param   items                Optional pointer to a JSON node containing
     *                               an object mapping property names to
     *                               schemas.
     * @param   additionalItems      Optional pointer to a JSON node containing
     *                               an additional properties schema or a
     *                               boolean value.
     * @param   currentScope         URI for current resolution scope
     * @param   itemsPath            JSON Pointer representing the path to
     *                               the 'items' node
     * @param   additionalItemsPath  JSON Pointer representing the path to
     *                               the 'additionalItems' node
     * @param   fetchDoc             Function to fetch remote JSON documents
     *                               (optional)
     * @param   docCache             Cache of resolved and fetched remote
     *                               documents
     * @param   schemaCache          Cache of populated schemas
     *
     * @return  pointer to a new ItemsConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::SingularItemsConstraint makeSingularItemsConstraint(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType &items,
        const opt::optional<std::string> currentScope,
        const std::string &itemsPath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache)
    {
        constraints::SingularItemsConstraint constraint;

        // Construct a Schema object for each item in the items array, if an
        // array is provided, or a single Schema object, in an object value is
        // provided. If the items constraint is not provided, then array items
        // will be validated against the additionalItems schema.
        if (items.isObject() || (version == kDraft7 && items.maybeBool())) {
            // If the items constraint contains an object value, then it
            // should contain a Schema that will be used to validate all
            // items in a target array. Any schema defined by the
            // additionalItems constraint will be ignored.
            const Subschema *subschema = makeOrReuseSchema<AdapterType>(
                    rootSchema, rootNode, items, currentScope, itemsPath,
                    fetchDoc, nullptr, nullptr, docCache, schemaCache);
            constraint.setItemsSubschema(subschema);

        } else if (items.maybeObject()) {
            // If a loosely-typed Adapter type is being used, then we'll
            // assume that an empty schema has been provided.
            constraint.setItemsSubschema(rootSchema.emptySubschema());

        } else {
            // All other formats will result in an exception being thrown.
            throw std::runtime_error(
                    "Expected valid schema for singular 'items' constraint.");
        }

        return constraint;
    }

    /**
     * @brief   Make a new MaximumConstraint object (draft 3 and 4).
     *
     * @param   rootSchema        The Schema instance, and root subschema,
     *                            through which other subschemas can be
     *                            created and modified
     * @param   rootNode          Reference to the node from which JSON
     *                            References will be resolved when they refer
     *                            to the current document; used for recursive
     *                            parsing of schemas
     * @param   node              JSON node containing the maximum value.
     * @param   exclusiveMaximum  Optional pointer to a JSON boolean value that
     *                            indicates whether maximum value is excluded
     *                            from the range of permitted values.
     *
     * @return  pointer to a new MaximumConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::MaximumConstraint makeMaximumConstraint(
        const AdapterType &node,
        const AdapterType *exclusiveMaximum)
    {
        if (!node.maybeDouble()) {
            throw std::runtime_error(
                    "Expected numeric value for maximum constraint.");
        }

        constraints::MaximumConstraint constraint;
        constraint.setMaximum(node.asDouble());

        if (exclusiveMaximum) {
            if (!exclusiveMaximum->maybeBool()) {
                throw std::runtime_error(
                        "Expected boolean value for exclusiveMaximum "
                        "constraint.");
            }

            constraint.setExclusiveMaximum(exclusiveMaximum->asBool());
        }

        return constraint;
    }

    /**
     * @brief   Make a new MaximumConstraint object that is always exclusive (draft 7).
     *
     * @param   node       JSON node containing an integer, representing the maximum value.
     *
     * @param   exclusive  Optional pointer to a JSON boolean value that indicates whether the
     *                     maximum value is excluded from the range of permitted values.
     *
     * @return  pointer to a new Maximum that belongs to the caller
     */
    template<typename AdapterType>
    constraints::MaximumConstraint makeMaximumConstraintExclusive(const AdapterType &node)
    {
        if (!node.maybeDouble()) {
            throw std::runtime_error("Expected numeric value for maximum constraint.");
        }

        constraints::MaximumConstraint constraint;
        constraint.setMaximum(node.asDouble());
        constraint.setExclusiveMaximum(true);
        return constraint;
    }

    /**
     * @brief   Make a new MaxItemsConstraint object.
     *
     * @param   node  JSON node containing an integer value representing the
     *                maximum number of items that may be contaned by an array.
     *
     * @return  pointer to a new MaxItemsConstraint that belongs to the caller.
     */
    template<typename AdapterType>
    constraints::MaxItemsConstraint makeMaxItemsConstraint(
        const AdapterType &node)
    {
        if (node.maybeInteger()) {
            const int64_t value = node.asInteger();
            if (value >= 0) {
                constraints::MaxItemsConstraint constraint;
                constraint.setMaxItems(value);
                return constraint;
            }
        }

        throw std::runtime_error(
                "Expected non-negative integer value for 'maxItems' "
                "constraint.");
    }

    /**
     * @brief   Make a new MaxLengthConstraint object.
     *
     * @param   node  JSON node containing an integer value representing the
     *                maximum length of a string.
     *
     * @return  pointer to a new MaxLengthConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::MaxLengthConstraint makeMaxLengthConstraint(
        const AdapterType &node)
    {
        if (node.maybeInteger()) {
            const int64_t value = node.asInteger();
            if (value >= 0) {
                constraints::MaxLengthConstraint constraint;
                constraint.setMaxLength(value);
                return constraint;
            }
        }

        throw std::runtime_error(
                "Expected a non-negative integer value for 'maxLength' "
                "constraint.");
    }

    /**
     * @brief   Make a new MaxPropertiesConstraint object.
     *
     * @param   node  JSON node containing an integer value representing the
     *                maximum number of properties that may be contained by an
     *                object.
     *
     * @return  pointer to a new MaxPropertiesConstraint that belongs to the
     *          caller
     */
    template<typename AdapterType>
    constraints::MaxPropertiesConstraint makeMaxPropertiesConstraint(
        const AdapterType &node)
    {
        if (node.maybeInteger()) {
            int64_t value = node.asInteger();
            if (value >= 0) {
                constraints::MaxPropertiesConstraint constraint;
                constraint.setMaxProperties(value);
                return constraint;
            }
        }

        throw std::runtime_error(
                "Expected a non-negative integer for 'maxProperties' "
                "constraint.");
    }

    /**
     * @brief  Make a new MinimumConstraint object (draft 3 and 4).
     *
     * @param  node              JSON node containing an integer, representing
     *                           the minimum value.
     *
     * @param  exclusiveMaximum  Optional pointer to a JSON boolean value that
     *                           indicates whether the minimum value is
     *                           excluded from the range of permitted values.
     *
     * @return  pointer to a new MinimumConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::MinimumConstraint makeMinimumConstraint(
        const AdapterType &node,
        const AdapterType *exclusiveMinimum)
    {
        if (!node.maybeDouble()) {
            throw std::runtime_error(
                    "Expected numeric value for minimum constraint.");
        }

        constraints::MinimumConstraint constraint;
        constraint.setMinimum(node.asDouble());

        if (exclusiveMinimum) {
            if (!exclusiveMinimum->maybeBool()) {
                throw std::runtime_error(
                        "Expected boolean value for 'exclusiveMinimum' "
                        "constraint.");
            }

            constraint.setExclusiveMinimum(exclusiveMinimum->asBool());
        }

        return constraint;
    }

    /**
     * @brief   Make a new MinimumConstraint object that is always exclusive (draft 7).
     *
     * @param   node       JSON node containing an integer, representing the minimum value.
     *
     * @param   exclusive  Optional pointer to a JSON boolean value that indicates whether the
     *                     minimum value is excluded from the range of permitted values.
     *
     * @return  pointer to a new MinimumConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::MinimumConstraint makeMinimumConstraintExclusive(const AdapterType &node)
    {
        if (!node.maybeDouble()) {
            throw std::runtime_error("Expected numeric value for minimum constraint.");
        }

        constraints::MinimumConstraint constraint;
        constraint.setMinimum(node.asDouble());
        constraint.setExclusiveMinimum(true);
        return constraint;
    }

    /**
     * @brief  Make a new MinItemsConstraint object.
     *
     * @param  node  JSON node containing an integer value representing the
     *               minimum number of items that may be contained by an array.
     *
     * @return  pointer to a new MinItemsConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::MinItemsConstraint makeMinItemsConstraint(
        const AdapterType &node)
    {
        if (node.maybeInteger()) {
            const int64_t value = node.asInteger();
            if (value >= 0) {
                constraints::MinItemsConstraint constraint;
                constraint.setMinItems(value);
                return constraint;
            }
        }

        throw std::runtime_error(
                "Expected a non-negative integer value for 'minItems' "
                "constraint.");
    }

    /**
     * @brief  Make a new MinLengthConstraint object.
     *
     * @param  node  JSON node containing an integer value representing the
     *               minimum length of a string.
     *
     * @return  pointer to a new MinLengthConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::MinLengthConstraint makeMinLengthConstraint(
        const AdapterType &node)
    {
        if (node.maybeInteger()) {
            const int64_t value = node.asInteger();
            if (value >= 0) {
                constraints::MinLengthConstraint constraint;
                constraint.setMinLength(value);
                return constraint;
            }
        }

        throw std::runtime_error(
                "Expected a non-negative integer value for 'minLength' "
                "constraint.");
    }


    /**
     * @brief   Make a new MaxPropertiesConstraint object.
     *
     * @param   node  JSON node containing an integer value representing the
     *                minimum number of properties that may be contained by an
     *                object.
     *
     * @return  pointer to a new MinPropertiesConstraint that belongs to the
     *          caller
     */
    template<typename AdapterType>
    constraints::MinPropertiesConstraint makeMinPropertiesConstraint(
        const AdapterType &node)
    {
        if (node.maybeInteger()) {
            int64_t value = node.asInteger();
            if (value >= 0) {
                constraints::MinPropertiesConstraint constraint;
                constraint.setMinProperties(value);
                return constraint;
            }
        }

        throw std::runtime_error(
                "Expected a non-negative integer for 'minProperties' "
                "constraint.");
    }

    /**
     * @brief   Make a new MultipleOfDoubleConstraint object
     *
     * @param   node  JSON node containing an numeric value that a target value
     *                must divide by in order to satisfy this constraint
     *
     * @return  a MultipleOfConstraint
     */
    template<typename AdapterType>
    constraints::MultipleOfDoubleConstraint makeMultipleOfDoubleConstraint(
        const AdapterType &node)
    {
        constraints::MultipleOfDoubleConstraint constraint;
        constraint.setDivisor(node.asDouble());
        return constraint;
    }

    /**
     * @brief   Make a new MultipleOfIntConstraint object
     *
     * @param   node  JSON node containing a numeric value that a target value
     *                must divide by in order to satisfy this constraint
     *
     * @return  a MultipleOfIntConstraint
     */
    template<typename AdapterType>
    constraints::MultipleOfIntConstraint makeMultipleOfIntConstraint(
            const AdapterType &node)
    {
        constraints::MultipleOfIntConstraint constraint;
        constraint.setDivisor(node.asInteger());
        return constraint;
    }

    /**
     * @brief   Make a new NotConstraint object
     *
     * @param   rootSchema    The Schema instance, and root subschema, through
     *                        which other subschemas can be created and modified
     * @param   rootNode      Reference to the node from which JSON References
     *                        will be resolved when they refer to the current
     *                        document; used for recursive parsing of schemas
     * @param   node          JSON node containing a schema
     * @param   currentScope  URI for current resolution scope
     * @param   nodePath      JSON Pointer representing path to current node
     * @param   fetchDoc      Function to fetch remote JSON documents (optional)
     * @param   docCache      Cache of resolved and fetched remote documents
     * @param   schemaCache   Cache of populated schemas
     *
     * @return  pointer to a new NotConstraint object that belongs to the caller
     */
    template<typename AdapterType>
    constraints::NotConstraint makeNotConstraint(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType &node,
        const opt::optional<std::string> currentScope,
        const std::string &nodePath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache)
    {
        if (node.maybeObject() || (version == kDraft7 && node.maybeBool())) {
            const Subschema *subschema = makeOrReuseSchema<AdapterType>(
                    rootSchema, rootNode, node, currentScope, nodePath,
                    fetchDoc, nullptr, nullptr, docCache, schemaCache);
            constraints::NotConstraint constraint;
            constraint.setSubschema(subschema);
            return constraint;
        }

        throw std::runtime_error("Expected object value for 'not' constraint.");
    }

    /**
     * @brief   Make a new OneOfConstraint object
     *
     * @param   rootSchema    The Schema instance, and root subschema, through
     *                        which other subschemas can be created and modified
     * @param   rootNode      Reference to the node from which JSON References
     *                        will be resolved when they refer to the current
     *                        document; used for recursive parsing of schemas
     * @param   node          JSON node containing an array of child schemas
     * @param   currentScope  URI for current resolution scope
     * @param   nodePath      JSON Pointer representing path to current node
     * @param   fetchDoc      Function to fetch remote JSON documents (optional)
     * @param   docCache      Cache of resolved and fetched remote documents
     * @param   schemaCache   Cache of populated schemas
     *
     * @return  pointer to a new OneOfConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::OneOfConstraint makeOneOfConstraint(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType &node,
        const opt::optional<std::string> currentScope,
        const std::string &nodePath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache)
    {
        constraints::OneOfConstraint constraint;

        int index = 0;
        for (const AdapterType schemaNode : node.getArray()) {
            const std::string childPath = nodePath + "/" +
                    std::to_string(index);
            const Subschema *subschema = makeOrReuseSchema<AdapterType>(
                rootSchema, rootNode, schemaNode, currentScope, childPath,
                fetchDoc, nullptr, nullptr, docCache, schemaCache);
            constraint.addSubschema(subschema);
            index++;
        }

        return constraint;
    }

    /**
     * @brief   Make a new PatternConstraint object.
     *
     * @param   node      JSON node containing a pattern string
     *
     * @return  pointer to a new PatternConstraint object that belongs to the
     *          caller
     */
    template<typename AdapterType>
    constraints::PatternConstraint makePatternConstraint(
        const AdapterType &node)
    {
        constraints::PatternConstraint constraint;
        constraint.setPattern(node.getString());
        return constraint;
    }

    /**
     * @brief   Make a new Properties object.
     *
     * @param   rootSchema                The Schema instance, and root
     *                                    subschema, through which other
     *                                    subschemas can be created and modified
     * @param   rootNode                  Reference to the node from which JSON
     *                                    References will be resolved when they
     *                                    refer to the current document; used
     *                                    for recursive parsing of schemas
     * @param   properties                Optional pointer to a JSON node
     *                                    containing an object mapping property
     *                                    names to schemas.
     * @param   patternProperties         Optional pointer to a JSON node
     *                                    containing an object mapping pattern
     *                                    property names to schemas.
     * @param   additionalProperties      Optional pointer to a JSON node
     *                                    containing an additional properties
     *                                    schema or a boolean value.
     * @param   currentScope              URI for current resolution scope
     * @param   propertiesPath            JSON Pointer representing the path to
     *                                    the 'properties' node
     * @param   patternPropertiesPath     JSON Pointer representing the path to
     *                                    the 'patternProperties' node
     * @param   additionalPropertiesPath  JSON Pointer representing the path to
     *                                    the 'additionalProperties' node
     * @param   fetchDoc                  Function to fetch remote JSON
     *                                    documents (optional)
     * @param   parentSubschema           Optional pointer to the Schema of the
     *                                    parent object, needed to support the
     *                                    'required' keyword in Draft 3
     * @param   docCache                  Cache of resolved and fetched remote
     *                                    documents
     * @param   schemaCache               Cache of populated schemas
     *
     * @return  pointer to a new Properties that belongs to the caller
     */
    template<typename AdapterType>
    constraints::PropertiesConstraint makePropertiesConstraint(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType *properties,
        const AdapterType *patternProperties,
        const AdapterType *additionalProperties,
        const opt::optional<std::string> currentScope,
        const std::string &propertiesPath,
        const std::string &patternPropertiesPath,
        const std::string &additionalPropertiesPath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        const Subschema *parentSubschema,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache)
    {
        typedef typename AdapterType::ObjectMember Member;

        constraints::PropertiesConstraint constraint;

        // Create subschemas for 'properties' constraint
        if (properties) {
            for (const Member m : properties->getObject()) {
                const std::string &property = m.first;
                const std::string childPath = propertiesPath + "/" + property;
                const Subschema *subschema = makeOrReuseSchema<AdapterType>(
                        rootSchema, rootNode, m.second, currentScope, childPath,
                        fetchDoc, parentSubschema, &property, docCache,
                        schemaCache);
                constraint.addPropertySubschema(property, subschema);
            }
        }

        // Create subschemas for 'patternProperties' constraint
        if (patternProperties) {
            for (const Member m : patternProperties->getObject()) {
                const std::string &pattern = m.first;
                const std::string childPath = patternPropertiesPath + "/" +
                        pattern;
                const Subschema *subschema = makeOrReuseSchema<AdapterType>(
                        rootSchema, rootNode, m.second, currentScope, childPath,
                        fetchDoc, parentSubschema, &pattern, docCache,
                        schemaCache);
                constraint.addPatternPropertySubschema(pattern, subschema);
            }
        }

        // Create an additionalItems subschema if required
        if (additionalProperties) {
            // If additionalProperties has been set, check for a boolean value.
            // Setting 'additionalProperties' to true allows the values of
            // additional properties to take any form. Setting it false
            // prohibits the use of additional properties.
            // If additionalProperties is instead an object, it should be
            // parsed as a schema. If additionalProperties has any other type,
            // then the schema is not valid.
            if (additionalProperties->isBool() ||
                additionalProperties->maybeBool()) {
                // If it has a boolean value that is 'true', then an empty
                // schema should be used.
                if (additionalProperties->asBool()) {
                    constraint.setAdditionalPropertiesSubschema(
                            rootSchema.emptySubschema());
                }
            } else if (additionalProperties->isObject()) {
                // If additionalProperties is an object, it should be used as
                // a child schema.
                const Subschema *subschema = makeOrReuseSchema<AdapterType>(
                        rootSchema, rootNode, *additionalProperties,
                        currentScope, additionalPropertiesPath, fetchDoc, nullptr,
                        nullptr, docCache, schemaCache);
                constraint.setAdditionalPropertiesSubschema(subschema);
            } else {
                // All other types are invalid
                throw std::runtime_error(
                        "Invalid type for 'additionalProperties' constraint.");
            }
        } else {
            // If an additionalProperties constraint is not provided, then the
            // default value is an empty schema.
            constraint.setAdditionalPropertiesSubschema(
                    rootSchema.emptySubschema());
        }

        return constraint;
    }

    template<typename AdapterType>
    constraints::PropertyNamesConstraint makePropertyNamesConstraint(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType &currentNode,
        const opt::optional<std::string> currentScope,
        const std::string &nodePath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache)
    {
        const Subschema *subschema = makeOrReuseSchema<AdapterType>(rootSchema, rootNode,
                currentNode, currentScope, nodePath, fetchDoc, nullptr, nullptr, docCache,
                schemaCache);

        constraints::PropertyNamesConstraint constraint;
        constraint.setSubschema(subschema);
        return constraint;
    }

    /**
     * @brief   Make a new RequiredConstraint.
     *
     * This function is used to create new RequiredContraint objects for
     * Draft 3 schemas.
     *
     * @param   node  Node containing a boolean value.
     * @param   name  Name of the required attribute.
     *
     * @return  pointer to a new RequiredConstraint object that belongs to the
     *          caller
     */
    template<typename AdapterType>
    opt::optional<constraints::RequiredConstraint>
            makeRequiredConstraintForSelf(const AdapterType &node,
                    const std::string &name)
    {
        if (!node.maybeBool()) {
            throw std::runtime_error("Expected boolean value for 'required' attribute.");
        }

        if (node.asBool()) {
            constraints::RequiredConstraint constraint;
            constraint.addRequiredProperty(name);
            return constraint;
        }

        return opt::optional<constraints::RequiredConstraint>();
    }

    /**
     * @brief   Make a new RequiredConstraint.
     *
     * This function is used to create new RequiredContraint objects for
     * Draft 4 schemas.
     *
     * @param   node  Node containing an array of strings.
     *
     * @return  pointer to a new RequiredConstraint object that belongs to the
     *          caller
     */
    template<typename AdapterType>
    constraints::RequiredConstraint makeRequiredConstraint(
        const AdapterType &node)
    {
        constraints::RequiredConstraint constraint;

        for (const AdapterType v : node.getArray()) {
            if (!v.maybeString()) {
                throw std::runtime_error("Expected required property name to "
                        "be a string value");
            }

            constraint.addRequiredProperty(v.getString());
        }

        return constraint;
    }

    /**
     * @brief   Make a new TypeConstraint object
     *
     * @param   rootSchema    The Schema instance, and root subschema, through
     *                        which other subschemas can be created and modified
     * @param   rootNode      Reference to the node from which JSON References
     *                        will be resolved when they refer to the current
     *                        document; used for recursive parsing of schemas
     * @param   node          Node containing the name of a JSON type
     * @param   currentScope  URI for current resolution scope
     * @param   nodePath      JSON Pointer representing path to current node
     * @param   fetchDoc      Function to fetch remote JSON documents (optional)
     * @param   docCache      Cache of resolved and fetched remote documents
     * @param   schemaCache   Cache of populated schemas
     *
     * @return  pointer to a new TypeConstraint object.
     */
    template<typename AdapterType>
    constraints::TypeConstraint makeTypeConstraint(
        Schema &rootSchema,
        const AdapterType &rootNode,
        const AdapterType &node,
        const opt::optional<std::string> currentScope,
        const std::string &nodePath,
        const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
        typename DocumentCache<AdapterType>::Type &docCache,
        SchemaCache &schemaCache)
    {
        typedef constraints::TypeConstraint TypeConstraint;

        TypeConstraint constraint;

        if (node.maybeString()) {
            const TypeConstraint::JsonType type =
                    TypeConstraint::jsonTypeFromString(node.getString());

            if (type == TypeConstraint::kAny && version == kDraft4) {
                throw std::runtime_error(
                        "'any' type is not supported in version 4 schemas.");
            }

            constraint.addNamedType(type);

        } else if (node.maybeArray()) {
            int index = 0;
            for (const AdapterType v : node.getArray()) {
                if (v.maybeString()) {
                    const TypeConstraint::JsonType type =
                            TypeConstraint::jsonTypeFromString(v.getString());

                    if (type == TypeConstraint::kAny && version == kDraft4) {
                        throw std::runtime_error(
                                "'any' type is not supported in version 4 "
                                "schemas.");
                    }

                    constraint.addNamedType(type);

                } else if (v.maybeObject() && version == kDraft3) {
                    const std::string childPath = nodePath + "/" +
                            std::to_string(index);
                    const Subschema *subschema = makeOrReuseSchema<AdapterType>(
                            rootSchema, rootNode, v, currentScope, childPath,
                            fetchDoc, nullptr, nullptr, docCache, schemaCache);
                    constraint.addSchemaType(subschema);

                } else {
                    throw std::runtime_error("Type name should be a string.");
                }

                index++;
            }

        } else if (node.maybeObject() && version == kDraft3) {
            const Subschema *subschema = makeOrReuseSchema<AdapterType>(
                    rootSchema, rootNode, node, currentScope, nodePath,
                    fetchDoc, nullptr, nullptr, docCache, schemaCache);
            constraint.addSchemaType(subschema);

        } else {
            throw std::runtime_error("Type name should be a string.");
        }

        return constraint;
    }

    /**
     * @brief   Make a new UniqueItemsConstraint object.
     *
     * @param   node  Node containing a boolean value.
     *
     * @return  pointer to a new UniqueItemsConstraint object that belongs to
     *          the caller, or nullptr if the boolean value is false.
     */
    template<typename AdapterType>
    opt::optional<constraints::UniqueItemsConstraint>
            makeUniqueItemsConstraint(const AdapterType &node)
    {
        if (node.isBool() || node.maybeBool()) {
            // If the boolean value is true, this function will return a pointer
            // to a new UniqueItemsConstraint object. If it is value, then the
            // constraint is redundant, so nullptr is returned instead.
            if (node.asBool()) {
                return constraints::UniqueItemsConstraint();
            } else {
                return opt::optional<constraints::UniqueItemsConstraint>();
            }
        }

        throw std::runtime_error(
                "Expected boolean value for 'uniqueItems' constraint.");
    }

};

}  // namespace valijson

#ifdef __clang__
#  pragma clang diagnostic pop
#endif
