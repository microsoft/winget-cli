// Copyright 2007-2010 Baptiste Lepilleur
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE

#ifndef CPPTL_JSON_READER_H_INCLUDED
#define CPPTL_JSON_READER_H_INCLUDED

#if !defined(JSON_IS_AMALGAMATION)
#include "features.h"
#include "value.h"
#endif // if !defined(JSON_IS_AMALGAMATION)
#include <deque>
#include <iosfwd>
#include <stack>
#include <string>
#include <istream>

// Disable warning C4251: <data member>: <type> needs to have dll-interface to
// be used by...
#if defined(JSONCPP_DISABLE_DLL_INTERFACE_WARNING)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif // if defined(JSONCPP_DISABLE_DLL_INTERFACE_WARNING)

namespace Json {

/** \brief Unserialize a <a HREF="http://www.json.org">JSON</a> document into a
 *Value.
 *
 * \deprecated Use CharReader and CharReaderBuilder.
 */
class JSON_API Reader {
public:
  typedef char Char;
  typedef const Char* Location;

  /** \brief Constructs a Reader allowing all features
   * for parsing.
   */
  Reader();

  /** \brief Constructs a Reader allowing the specified feature set
   * for parsing.
   */
  Reader(const Features& features);

  /** \brief Read a Value from a <a HREF="http://www.json.org">JSON</a>
   * document.
   * \param document UTF-8 encoded string containing the document to read.
   * \param root [out] Contains the root value of the document if it was
   *             successfully parsed.
   * \param collectComments \c true to collect comment and allow writing them
   * back during
   *                        serialization, \c false to discard comments.
   *                        This parameter is ignored if
   * Features::allowComments_
   *                        is \c false.
   * \return \c true if the document was successfully parsed, \c false if an
   * error occurred.
   */
  bool
  parse(const std::string& document, Value& root, bool collectComments = true);

  /** \brief Read a Value from a <a HREF="http://www.json.org">JSON</a>
   document.
   * \param beginDoc Pointer on the beginning of the UTF-8 encoded string of the
   document to read.
   * \param endDoc Pointer on the end of the UTF-8 encoded string of the
   document to read.
   *               Must be >= beginDoc.
   * \param root [out] Contains the root value of the document if it was
   *             successfully parsed.
   * \param collectComments \c true to collect comment and allow writing them
   back during
   *                        serialization, \c false to discard comments.
   *                        This parameter is ignored if
   Features::allowComments_
   *                        is \c false.
   * \return \c true if the document was successfully parsed, \c false if an
   error occurred.
   */
  bool parse(const char* beginDoc,
             const char* endDoc,
             Value& root,
             bool collectComments = true);

  /// \brief Parse from input stream.
  /// \see Json::operator>>(std::istream&, Json::Value&).
  bool parse(std::istream& is, Value& root, bool collectComments = true);

  /** \brief Returns a user friendly string that list errors in the parsed
   * document.
   * \return Formatted error message with the list of errors with their location
   * in
   *         the parsed document. An empty string is returned if no error
   * occurred
   *         during parsing.
   * \deprecated Use getFormattedErrorMessages() instead (typo fix).
   */
  JSONCPP_DEPRECATED("Use getFormattedErrorMessages instead")
  std::string getFormatedErrorMessages() const;

  /** \brief Returns a user friendly string that list errors in the parsed
   * document.
   * \return Formatted error message with the list of errors with their location
   * in
   *         the parsed document. An empty string is returned if no error
   * occurred
   *         during parsing.
   */
  std::string getFormattedErrorMessages() const;

private:
  enum TokenType {
    tokenEndOfStream = 0,
    tokenObjectBegin,
    tokenObjectEnd,
    tokenArrayBegin,
    tokenArrayEnd,
    tokenString,
    tokenNumber,
    tokenTrue,
    tokenFalse,
    tokenNull,
    tokenArraySeparator,
    tokenMemberSeparator,
    tokenComment,
    tokenError
  };

  class Token {
  public:
    TokenType type_;
    Location start_;
    Location end_;
  };

  class ErrorInfo {
  public:
    Token token_;
    std::string message_;
    Location extra_;
  };

  typedef std::deque<ErrorInfo> Errors;

  bool readToken(Token& token);
  void skipSpaces();
  bool match(Location pattern, int patternLength);
  bool readComment();
  bool readCStyleComment();
  bool readCppStyleComment();
  bool readString();
  void readNumber();
  bool readValue();
  bool readObject(Token& token);
  bool readArray(Token& token);
  bool decodeNumber(Token& token);
  bool decodeNumber(Token& token, Value& decoded);
  bool decodeString(Token& token);
  bool decodeString(Token& token, std::string& decoded);
  bool decodeDouble(Token& token);
  bool decodeDouble(Token& token, Value& decoded);
  bool decodeUnicodeCodePoint(Token& token,
                              Location& current,
                              Location end,
                              unsigned int& unicode);
  bool decodeUnicodeEscapeSequence(Token& token,
                                   Location& current,
                                   Location end,
                                   unsigned int& unicode);
  bool addError(const std::string& message, Token& token, Location extra = 0);
  bool recoverFromError(TokenType skipUntilToken);
  bool addErrorAndRecover(const std::string& message,
                          Token& token,
                          TokenType skipUntilToken);
  void skipUntilSpace();
  Value& currentValue();
  Char getNextChar();
  void
  getLocationLineAndColumn(Location location, int& line, int& column) const;
  std::string getLocationLineAndColumn(Location location) const;
  void addComment(Location begin, Location end, CommentPlacement placement);
  void skipCommentTokens(Token& token);

  typedef std::stack<Value*> Nodes;
  Nodes nodes_;
  Errors errors_;
  std::string document_;
  Location begin_;
  Location end_;
  Location current_;
  Location lastValueEnd_;
  Value* lastValue_;
  std::string commentsBefore_;
  Features features_;
  bool collectComments_;
};  // Reader

/** Interface for reading JSON from a char array.
 */
class JSON_API CharReader {
public:
  virtual ~CharReader() {}
  /** \brief Read a Value from a <a HREF="http://www.json.org">JSON</a>
   document.
   * The document must be a UTF-8 encoded string containing the document to read.
   *
   * \param beginDoc Pointer on the beginning of the UTF-8 encoded string of the
   document to read.
   * \param endDoc Pointer on the end of the UTF-8 encoded string of the
   document to read.
   *        Must be >= beginDoc.
   * \param root [out] Contains the root value of the document if it was
   *             successfully parsed.
   * \param errs [out] Formatted error messages (if not NULL)
   *        a user friendly string that lists errors in the parsed
   * document.
   * \return \c true if the document was successfully parsed, \c false if an
   error occurred.
   */
  virtual bool parse(
      char const* beginDoc, char const* endDoc,
      Value* root, std::string* errs) = 0;

  class Factory {
  public:
    /** \brief Allocate a CharReader via operator new().
     * \throw std::exception if something goes wrong (e.g. invalid settings)
     */
    virtual CharReader* newCharReader() const = 0;
  };  // Factory
};  // CharReader

/** \brief Build a CharReader implementation.

  \deprecated This is experimental and will be altered before the next release.

Usage:
\code
  using namespace Json;
  CharReaderBuilder builder;
  builder["collectComments"] = false;
  Value value;
  std::string errs;
  bool ok = parseFromStream(builder, std::cin, &value, &errs);
\endcode
*/
class JSON_API CharReaderBuilder : public CharReader::Factory {
public:
  // Note: We use a Json::Value so that we can add data-members to this class
  // without a major version bump.
  /** Configuration of this builder.
    These are case-sensitive.
    Available settings (case-sensitive):
    - `"collectComments": false or true`
      - true to collect comment and allow writing them
        back during serialization, false to discard comments.
        This parameter is ignored if allowComments is false.
    - `"allowComments": false or true`
      - true if comments are allowed.
    - `"strictRoot": false or true`
      - true if root must be either an array or an object value
    - `"allowDroppedNullPlaceholders": false or true`
      - true if dropped null placeholders are allowed. (See StreamWriterBuilder.)
    - `"allowNumericKeys": false or true`
      - true if numeric object keys are allowed.
    - `"allowSingleQuotes": false or true`
      - true if '' are allowed for strings (both keys and values)
    - `"stackLimit": integer`
      - Exceeding stackLimit (recursive depth of `readValue()`) will
        cause an exception.
      - This is a security issue (seg-faults caused by deeply nested JSON),
        so the default is low.
    - `"failIfExtra": false or true`
      - If true, `parse()` returns false when extra non-whitespace trails
        the JSON value in the input string.
    - `"rejectDupKeys": false or true`
      - If true, `parse()` returns false when a key is duplicated within an object.

    You can examine 'settings_` yourself
    to see the defaults. You can also write and read them just like any
    JSON Value.
    \sa setDefaults()
    */
  Json::Value settings_;

  CharReaderBuilder();
  virtual ~CharReaderBuilder();

  virtual CharReader* newCharReader() const;

  /** \return true if 'settings' are legal and consistent;
   *   otherwise, indicate bad settings via 'invalid'.
   */
  bool validate(Json::Value* invalid) const;

  /** A simple way to update a specific setting.
   */
  Value& operator[](std::string key);

  /** Called by ctor, but you can use this to reset settings_.
   * \pre 'settings' != NULL (but Json::null is fine)
   * \remark Defaults:
   * \snippet src/lib_json/json_reader.cpp CharReaderBuilderStrictMode
   */
  static void setDefaults(Json::Value* settings);
  /** Same as old Features::strictMode().
   * \pre 'settings' != NULL (but Json::null is fine)
   * \remark Defaults:
   * \snippet src/lib_json/json_reader.cpp CharReaderBuilderDefaults
   */
  static void strictMode(Json::Value* settings);
};

/** Consume entire stream and use its begin/end.
  * Someday we might have a real StreamReader, but for now this
  * is convenient.
  */
bool JSON_API parseFromStream(
    CharReader::Factory const&,
    std::istream&,
    Value* root, std::string* errs);

/** \brief Read from 'sin' into 'root'.

 Always keep comments from the input JSON.

 This can be used to read a file into a particular sub-object.
 For example:
 \code
 Json::Value root;
 cin >> root["dir"]["file"];
 cout << root;
 \endcode
 Result:
 \verbatim
 {
 "dir": {
     "file": {
     // The input stream JSON would be nested here.
     }
 }
 }
 \endverbatim
 \throw std::exception on parse error.
 \see Json::operator<<()
*/
JSON_API std::istream& operator>>(std::istream&, Value&);

} // namespace Json

#if defined(JSONCPP_DISABLE_DLL_INTERFACE_WARNING)
#pragma warning(pop)
#endif // if defined(JSONCPP_DISABLE_DLL_INTERFACE_WARNING)

#endif // CPPTL_JSON_READER_H_INCLUDED
