#include "handler_test.h"
#include "yaml-cpp/yaml.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::_;

namespace YAML {
namespace {

typedef HandlerTest GenEmitterTest;

TEST_F(GenEmitterTest, testf2a8b6e6359fb2c30830) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << "foo";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa2c9c04eab06a05bf1a3) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << "foo";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc5fae995bf84b2f62627) {
  Emitter out;
  out << BeginDoc;
  out << "foo";
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test208b4fb7399a936fce93) {
  Emitter out;
  out << BeginDoc;
  out << "foo";
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test402085442ada9788bc4e) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << "foo";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test279346c761f7d9a10aec) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << "foo";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test386f6766d57a48ccb316) {
  Emitter out;
  out << BeginDoc;
  out << "foo";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test989baa41ede860374193) {
  Emitter out;
  out << BeginDoc;
  out << "foo";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test718fa11b9bfa9dfc6632) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << "foo\n";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7986c74c7cab2ff062e7) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << "foo\n";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1a432be0760ebcf72dde) {
  Emitter out;
  out << BeginDoc;
  out << "foo\n";
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9b4714c8c6dd71f963f1) {
  Emitter out;
  out << BeginDoc;
  out << "foo\n";
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test59d039102f43b05233b2) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << "foo\n";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test15371be5fc126b3601ee) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << "foo\n";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test5a2a5702a41d71567a10) {
  Emitter out;
  out << BeginDoc;
  out << "foo\n";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7975acd31f55f66c21a9) {
  Emitter out;
  out << BeginDoc;
  out << "foo\n";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9ab358e41e3af0e1852c) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test6571b17e1089f3f34d41) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << "foo";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7c8476d0a02eeab3326f) {
  Emitter out;
  out << BeginDoc;
  out << VerbatimTag("tag");
  out << "foo";
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0883fa5d170d96324325) {
  Emitter out;
  out << BeginDoc;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7f44d870f57878e83749) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << VerbatimTag("tag");
  out << "foo";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc7b8d9af2a71da438220) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << "foo";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa27a4f0174aee7622160) {
  Emitter out;
  out << BeginDoc;
  out << VerbatimTag("tag");
  out << "foo";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf06e77dc66bc51682e57) {
  Emitter out;
  out << BeginDoc;
  out << VerbatimTag("tag");
  out << "foo";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test28c636f42558c217d90b) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << Anchor("anchor");
  out << "foo";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa8e930c2f4f761519825) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << Anchor("anchor");
  out << "foo";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc73b721f492b45035034) {
  Emitter out;
  out << BeginDoc;
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb401b54145c71ea07848) {
  Emitter out;
  out << BeginDoc;
  out << Anchor("anchor");
  out << "foo";
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test380d9af0ae2e27279526) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << Anchor("anchor");
  out << "foo";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test6969308096c6106d1f55) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << Anchor("anchor");
  out << "foo";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa8afc0036fffa3b2d185) {
  Emitter out;
  out << BeginDoc;
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7b41f0a32b90bf5f138d) {
  Emitter out;
  out << BeginDoc;
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test99b1e0027d74c641f4fc) {
  Emitter out;
  out << Comment("comment");
  out << "foo";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test8e45fdb3ff3c00d56f27) {
  Emitter out;
  out << Comment("comment");
  out << "foo";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf898ade0c92d48d498cf) {
  Emitter out;
  out << "foo";
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3eb11fe6897b9638b90c) {
  Emitter out;
  out << "foo";
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test4e7428248511a461fdae) {
  Emitter out;
  out << Comment("comment");
  out << "foo";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb811cba8e9f57399cd40) {
  Emitter out;
  out << Comment("comment");
  out << "foo";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc625669ef35d9165757f) {
  Emitter out;
  out << "foo";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0bc005214f48707274f7) {
  Emitter out;
  out << "foo";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testdccc5288dfb2f680dd82) {
  Emitter out;
  out << Comment("comment");
  out << "foo\n";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0a928620b149d5644a3b) {
  Emitter out;
  out << Comment("comment");
  out << "foo\n";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test72bf9f8ba5207fb041c3) {
  Emitter out;
  out << "foo\n";
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test39ba33ec287e431e70d0) {
  Emitter out;
  out << "foo\n";
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testce39fe4e650942b617c6) {
  Emitter out;
  out << Comment("comment");
  out << "foo\n";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test680e99eab986e1cdac01) {
  Emitter out;
  out << Comment("comment");
  out << "foo\n";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste6e7f73dfac0048154af) {
  Emitter out;
  out << "foo\n";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test19e2c91493d21a389511) {
  Emitter out;
  out << "foo\n";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "!", 0, "foo\n"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc15cdbdbf9661c853def) {
  Emitter out;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << "foo";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa349878c464f03fa6d4e) {
  Emitter out;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << "foo";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc063a846f87b1e20e4e9) {
  Emitter out;
  out << VerbatimTag("tag");
  out << "foo";
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testdfb3a9ec6da3b792f392) {
  Emitter out;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test6f545990782be38424bf) {
  Emitter out;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << "foo";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9d7dd5e044527a4e8f31) {
  Emitter out;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << "foo";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testba570ae83f89342779ff) {
  Emitter out;
  out << VerbatimTag("tag");
  out << "foo";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc6fc50c169793aa60531) {
  Emitter out;
  out << VerbatimTag("tag");
  out << "foo";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testd10a9c9671992acd494d) {
  Emitter out;
  out << Comment("comment");
  out << Anchor("anchor");
  out << "foo";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testd5ee8a3bdb42c8639ad4) {
  Emitter out;
  out << Comment("comment");
  out << Anchor("anchor");
  out << "foo";
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test24914f6c2b7f7d5843c4) {
  Emitter out;
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9af19fe8c77aa18cd462) {
  Emitter out;
  out << Anchor("anchor");
  out << "foo";
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc2f9274717aaf39b0838) {
  Emitter out;
  out << Comment("comment");
  out << Anchor("anchor");
  out << "foo";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test75f3a7f62b5b77411653) {
  Emitter out;
  out << Comment("comment");
  out << Anchor("anchor");
  out << "foo";

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testea51373e6b4e598b2adf) {
  Emitter out;
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa594204f0724101d4931) {
  Emitter out;
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test87638f2fba55c5235720) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginSeq;
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test786f027ec8e380bdeb45) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginSeq;
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9d9ca2fc29536ef5d392) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Comment("comment");
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testde9c33927d8f706e5191) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << EndSeq;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa03392eb1d9af2180eb2) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << EndSeq;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test8826ba8a4954e2775441) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf116d1ab8a1646bb4295) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1d4afe394248c5d6f190) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test4cc7b190d6dd08368f08) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc623063380afa67c57c4) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste24ef3f378c4c33107b2) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test2aa8f68b872fd07fde8c) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test940bf9330572d48b476f) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testd710bce67052a991abfa) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testd2ac557dae648cd1ba66) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb394f0e282404d1235d3) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testaf620080909b118a715d) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testfc23fc6f424006e5907f) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testbc5517fe466dd4988ce2) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc0db52f1be33ddf93852) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test279a9eef5b2d2cf98ecf) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7f55b2f00c1090e43af5) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1be996b4b790d9bd565d) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa5dea69e968ea27412cc) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test77fe0d4370db4aa8af1a) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test6d0319a28dd1a931f211) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0031c4cd5331366162a6) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc0c74d483811e3322ed2) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0d0938c9dca1e78401d6) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0c83b8f0404e62673099) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test733fd2f94ae082ea6076) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test04b57f98a492b0f2c1ad) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test80d83a80f235341f1bff) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testecbe137bf7436ccd7976) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf14f2e8202cacdf9252d) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9c029f7cf565580a56fd) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test129cd28cda34c7b97a89) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Comment("comment");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1c55ee081412be96e00f) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf9e9d15d9e09a8e98681) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test03dd7104722840fe7fee) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste6c856a08270255404b6) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf285ed8797058c0e4e2f) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test906076647b894281787e) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Comment("comment");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test8a836336041c56130c5c) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc7f61ada097fb34f24ce) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testec075d926fd1f95a1bae) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa79ce9edc0c3593faa31) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test525c133ebf8f46a1962f) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb06604d03a8c9cfbe7c2) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste268ba5f2d54120eb665) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7a646350a81bba70e44a) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test025df570e0d8a1f818da) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test897087b9aba1d5773870) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa45ee0501da4a4e5da54) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste751c06ea558ccca1821) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test8526d26e85cc930eecec) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste9a5a4a0f0e44311d01a) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testac8a091ab93b65aee893) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testee014788f524623b5075) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test57a067545c01c42a7b4e) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test948ac02da8825214c869) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa3d6c5e8a1658c1dd726) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test548d71006d7cafde91da) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test35e08ea7459dbee9eab8) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test87e79665a4339434d781) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test4928d09bc979129c05ca) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1d2f73011af6b4486504) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test2460718f7277d5f42306) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test52309e87b3f0185f982b) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa51d8f1cedfead1de5ab) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test537bf14b4d578f212f4d) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste19e3fd4d5cd52bf6754) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf27e53142f2ca0e96a99) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test8ce13fdbb0e53e131cbe) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9fa693277f014353aa34) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc3e4849fb38bc3556f45) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test34049495795f40da2d52) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << Comment("comment");
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test14353701fc865919ab50) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test74547fc0ba8d387c5423) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test52d2b69b185f6ccfff4c) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test44d442585e5bc9a7644a) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3dc263684801dec471c9) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa04cde3245ad9b929b9a) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testd911e740ca36e0509dfa) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testde44215fe9b2e87846ba) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test6390021323a4889f19d2) {
  Emitter out;
  out << BeginDoc;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1db2fcb7347f6cb37dd4) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test06b32e9d75498ee291d2) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test86654989004963952b15) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test53d875fc5058faa44a4e) {
  Emitter out;
  out << BeginSeq;
  out << EndSeq;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3f4b49a82b6e07eb11fd) {
  Emitter out;
  out << BeginSeq;
  out << EndSeq;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testae4e2fa09d6a34077b6e) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb181b63559d96d5f848c) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test817661fec7d3730f4fa6) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test34bb2700e9688718fa5a) {
  Emitter out;
  out << BeginSeq;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test84e3c1999b6888d2e897) {
  Emitter out;
  out << BeginSeq;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa9d113656780031a99f5) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1cd1ead50aaa7b827068) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1389f95066b07eac89ef) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test709f3a5c294f47f62c1e) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test8a238d7fdee02a368203) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0d13534e2949ea35ca96) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test10fe6827ed46e0e063a7) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc7eb6d9da57005534c1c) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3f424efd76e1d32727eb) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test2bdc361bc6b056f02465) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0cc1936afe5637ba1376) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7d3e2f793963d3480545) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf3f50e76d7ef6e2b2bff) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testcbf1cff67fec9148df1c) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test168bd4b8dc78b4d524ee) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb616ef26030304bca6ef) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9fda976f36ddb23b38ee) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test48e8c45c081edc86deb2) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test30f5136e817ddd8158de) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testeb51d66281f593566172) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testef6ffa5fa4658785ef00) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test6db34efc6b59e8a7ba18) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test537b9ecc9d9a5b546a9c) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testfadd6ee259c13382f5ce) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "tag", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test974ae82483391d01787b) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7fc68b49cfe198b30eeb) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test41644c59ff95f8ec5ec2) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa3a24413b537aece4834) {
  Emitter out;
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc4516128af938868b120) {
  Emitter out;
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testef3c20a56c8a3993cc2d) {
  Emitter out;
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test83aceee2ee6446347fba) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test5a054d76c67b6de340e2) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc6706e6b6fc94d1e4752) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test72f3ded341d6b5d21803) {
  Emitter out;
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7dc830828b604b5d1839) {
  Emitter out;
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3a5baef0d6a62e5880ef) {
  Emitter out;
  out << BeginSeq;
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "?", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testfe7bf25b7a5525cab12a) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test817bf3d583230e503f8e) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testab122e386b3e30ea59e2) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test466c3e0dbec8e9660837) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9fc49f92e554cd85e349) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf9d2f39bdbd217d70868) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1ce3d77707f18ec48a19) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test71df6ecc32e49ea961d4) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test8f37b0a6cc287f8c922f) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf992e2a1f7d737647506) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testd79381f97cdd0af81ae4) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test74ca1feb5f0c520a8518) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste86e6fd56707272c091b) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1e6f73bc378c184c786b) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3fbac5e1aef66dc40bf7) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test558c4bf1c9c6e4e81e98) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testfa6d88b26c0072cddb26) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test40a5af3360fb3d9e79f1) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test451dd95b95b7e958bb03) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1717ad2d772bafb9b573) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testedc6737e8b2f5b23b42e) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test771c7d28c0b8c184e2c7) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test469a446f0b22e9b6d269) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testec45b0503f312be47336) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1bfc4f39d6730acb6a12) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9bc9a72ad06084dc8cf8) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << "bar";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test62c996cdfc1d3b77b7ec) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1d038936a340d5bef490) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7057f64ac570dbe3c1ca) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testbfe0890de3ffc73f0f9d) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test5faa7320a493247b4f8b) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << Comment("comment");
  out << EndSeq;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test929fbc93b3d6d98b1f0a) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testcc7d1ad7797581b37549) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1115ba981ba8f739ddf2) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf7ca743a82040e1313a8) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa4e0257ad6c987178ca4) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb65ceea0d4080b44180e) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test4fcd60d48dbd7b07e289) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test92704937d4e130b43390) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test029a31902f93dfa9ea7b) {
  Emitter out;
  out << BeginSeq;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test40b4e7494e5b850d26f4) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginMap;
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test64d2ab5993b67281212b) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginMap;
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste71b9b975d71c18a2897) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << Comment("comment");
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test138039761e432a5ba11e) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << EndMap;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test6684d2eacb3f094bfc84) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << EndMap;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test8624a705f2167d4db358) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test90877a1ec609edb69bce) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test5f925d3c910a7e32bb99) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << Comment("comment");
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testffeb4955bf4ee9510a88) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test769ee82c3bfc52d7a85d) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testdc4e16b5a48fe16102b4) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testd3c578e5b5a6813a73c7) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0034b2c9905b34f7f22e) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste911e620becf080a4d96) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7f8bbf619609651a2e55) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test2974bda177bed72619f4) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testbc7a1599883ed8c27262) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test323e14a02e02b94939fb) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test705ff113324bf0b4897c) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test587f5739ba58f0e21e0e) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test31a8c7da96ebe3da3f6e) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0b6fe270e4cf9fc21181) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testaf5869c722ea0dfb3394) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc348837f92793a778246) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9d26ae9ec8db76a06a6f) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test28691969bbaa41191640) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb38c27cd2556a14bb479) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1103d3c99e3525075da6) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testeb7edb5d1dfd039c72c3) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa9862d708fcb755db479) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testae3e98286336f0c5d2af) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test8bd970000ae21619e864) {
  Emitter out;
  out << Comment("comment");
  out << BeginDoc;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0a960fe3efeeb1b4fafe) {
  Emitter out;
  out << BeginDoc;
  out << Comment("comment");
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testffec1dcba9a2622b57a3) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9a181b6042027e7977bf) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test5b42728bff7e0dd63ae8) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << Comment("comment");
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa17514c4db3a70fe5084) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test2ac903a52c526db4b34b) {
  Emitter out;
  out << BeginDoc;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testbebc6bc66d04a91bfa9c) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0918e247384bfc94d831) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf8512b2ebdaad8ae4cae) {
  Emitter out;
  out << BeginMap;
  out << Comment("comment");
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test01a1d249079c380030ca) {
  Emitter out;
  out << BeginMap;
  out << EndMap;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testcb48737e9c352108dc56) {
  Emitter out;
  out << BeginMap;
  out << EndMap;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testdea8106f3dce46929197) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test2b91aa87abdaa0fc0b20) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9c8b1fe0c5bbbf6a787e) {
  Emitter out;
  out << BeginMap;
  out << Comment("comment");
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test8808d4be9571f365f79a) {
  Emitter out;
  out << BeginMap;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste77c95c5163513fa25c5) {
  Emitter out;
  out << BeginMap;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa3ed6e26dac366240579) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test136adbd0ad47d74cfa22) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test77f384e8387e39b54691) {
  Emitter out;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf8f016177cf9e428fcd4) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1cec69d3c95937f4137a) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb1fe1a5c5c064bdfe505) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test14200fdf4de8797d8dfb) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testde7595a96199f66d7ac0) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb1434e1f508509c0ade4) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0d3bd788298201abbe67) {
  Emitter out;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3c716f5c232001f04805) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa55ef29eecbda5bc5b69) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test55ddd593defa5ee8da90) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7326f87fd5c3adff317b) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test5ebc413d376f3b965879) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test4c7159334e528e2cfff8) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test6a6bc06cdfee9f58a094) {
  Emitter out;
  out << BeginMap;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0beedfaace1b1e71d0c6) {
  Emitter out;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9f65fafc369193908b7b) {
  Emitter out;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3a5c3ac504d7a58a08ca) {
  Emitter out;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << EndDoc;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc007513c868038dd3a68) {
  Emitter out;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << EndDoc;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test89f3ba065cbd341381ec) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test692c2652cee84e90c096) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test761fba62f7617a03fbf0) {
  Emitter out;
  out << BeginMap;
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste960a69bc06912eb8c76) {
  Emitter out;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << Comment("comment");
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test8f6187c6c2419dbf1770) {
  Emitter out;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << Comment("comment");
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testba6cb3810a074fabc55e) {
  Emitter out;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testac695c3621ec3f104672) {
  Emitter out;
  out << BeginMap;
  out << VerbatimTag("tag");
  out << Anchor("anchor");
  out << "foo";
  out << VerbatimTag("tag");
  out << Anchor("other");
  out << "bar";
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnAnchor(_, "anchor"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 1, "foo"));
  EXPECT_CALL(handler, OnAnchor(_, "other"));
  EXPECT_CALL(handler, OnScalar(_, "tag", 2, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test86494a6bcb6a65e7029e) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb406fb13323c199d709c) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test4409f685a3e80b9ab415) {
  Emitter out;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa74ace9c1f5e18cf3f2a) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testabfc8ce2ca4c3dafa013) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << "foo";
  out << Comment("comment");
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test07ff4bbae6104c4e30c1) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << "foo";
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1e3559cacab6d46c98fe) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << "foo";
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test795830e12e5a20213a7e) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test849f2c88c71734fcf3f3) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7bb139ac1f14e8ae04e2) {
  Emitter out;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test58655e359c60bf73986f) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testde9f70648448cbd37245) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testfff25037c90a64db8771) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test94b24a286074cac9b881) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0c678fe6c6dbd3ccf4eb) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test15f6ce577f139b9f1b61) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test5e927c8865f44b5c1abe) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test235aebc60786962899f1) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test45e109e1bc3ca312091d) {
  Emitter out;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9a58086d44719b21c6b3) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa3ac3fa06ae69e9f5c9d) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9ccaf628d78cc8f27857) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test6b8483101720027fd945) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1549a08694b053f8a2cb) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0417f66daae22676ad66) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test6b3d4384d27d0ac90b01) {
  Emitter out;
  out << BeginMap;
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testbf166fa245c204799ea8) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testdf279b50896e8f084ed3) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7d9b55fe33dfdc6cf930) {
  Emitter out;
  out << BeginMap;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test2b0f0825ac5d9ac3baf7) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste4865b227f48a727aafe) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb02fb812ae14499cc30e) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa395515c1bce39e737b7) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << "foo";
  out << Comment("comment");
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test813be458848be1ba3bf1) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << "foo";
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test23ab6af7b3fc7957a03d) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << "foo";
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc5fb40e239029d9efa58) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test88b5c599a4f9ac52f951) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3995578993108fa25f88) {
  Emitter out;
  out << BeginMap;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test61a24036be6f8cd49a28) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc13e02ab3c1bd1db6e55) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb3ca69f6d7a888644064) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test4e6337821c858c2f7cfa) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test8dc4a01d956c779dd8b0) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1dae9c6350559f6b9f89) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9f891b1cdde286863956) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test0704315052c50c54b17a) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7f5b47cf1d2571afc033) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test5040c19e850e3046d32d) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test110665e2f3409ef307ff) {
  Emitter out;
  out << BeginMap;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste9e1549f96267f93651c) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test6a5b6a20fdb6f7c3279e) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7c568c68f77e34d5c714) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testfd8fe783b5297c92d17f) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test418b1426e630825a7c85) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test8161c5e486d317e7864e) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa48e1915ca7c919db445) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test27124815aea27c053aab) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste751377e4a74306bc555) {
  Emitter out;
  out << BeginMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test36a4cc298255efdd1ef5) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test43471dee97b0506909b2) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test959c85b4e833f72173a6) {
  Emitter out;
  out << BeginMap;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3a09723dce29399bc865) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test958e404277a1c55139af) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb1823407ab0601edc9cb) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9993e9dad983b28960aa) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << "foo";
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test8e87f559b25a9a20e11c) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << "foo";
  out << Comment("comment");
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testeb431be8504451636efe) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << "foo";
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf876965882befc7355df) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << "foo";
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf7e1d47f266f0b940fed) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste5a4bc646f2182b78cc1) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test6cbedca25c9a8a6bb42e) {
  Emitter out;
  out << BeginMap;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test07613cc34874a5b47577) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa55b7ac19580aeb82d32) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test458f9af92dfb9f64a488) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc4a9c3769e95770bb455) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testae56654f89fa2416f3db) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1635f71f27e7303c730e) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testcd9d835a8d8b8622c63b) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3c2d1f91e57040cb8fdd) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9bd5b4c002b3b637747f) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf6c71199cd45b5b8e7e0) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test865b02b69742e5513963) {
  Emitter out;
  out << Comment("comment");
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test86eb8783dc4367cda931) {
  Emitter out;
  out << BeginMap;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7f6604058a9d8e2e1219) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test676b4abcb3cf0530e4da) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste7e52980e73d442a602b) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc3d6f480087db4bcd3a1) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test6943516ed43da6944e96) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb7e880522d0778ae6e6f) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testec39ee51992618c7b154) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test39f768713a9b3aaffe0d) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << EndMap;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf287a68abc5b8ff7784d) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc36154aa87842ba9699f) {
  Emitter out;
  out << BeginMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndMap;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa75da84dfc8ee5507157) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc54a03d1735615f7bd60) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf5d72aba828875527d6f) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9e8681a1b27a8524ec5e) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test34678928e79e6eb160f4) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << "foo";
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test07c6d6b9e133553d4532) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1531642db71e1aa8dd1c) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test22fde02c0bd0ecb8a527) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test72430574ba42559cf917) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test067bce5d8aa6281a3e6f) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb1839ee8ced911bb1ed1) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test5899c9cd7e5a40077178) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test834f4e4a74c0ac6cd011) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testd3aeee7524918cf227e7) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7c587549aa0bbd6e2d53) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc684ba00d512b6009b02) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testbe39189b638b9e0214dd) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test80326240018ececfa606) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test8a74653a376d02a2b6db) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testaa82cace20492eb66f60) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testcd4a1cdb4e2a24cae5c1) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testa9ef5ab0eada79175f6a) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test4060ba4b4f9b8193dcc4) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test6cd2fc4be08857654fa0) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testadb892f6183cde28d9cc) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test5e830445d6cafe856b09) {
  Emitter out;
  out << BeginSeq;
  out << "foo";
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test16a7d875f6358bdd36ee) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test75b4342605739c88bc3f) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test7d42488f1a02d045d278) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9ce404764c03c6644c98) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf9e413cd5405efcbd432) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test757c82faa95e2e507ee9) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb73d72bf5de11964969f) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3926f169b9dce6db913f) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test77b0cc7e618a09e0556d) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test6817f3e5da2ad8823025) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test817ae48b78359d60888b) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9db1bf6278dd7de937e6) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test4d5ca5c891442ddf7e84) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testfb6eb22f4bf080b9ac8b) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3ce4b4ec89282d701502) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testaf53ae415739a8812200) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test72d3de78c6508500cb00) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test6dd3d3703718b37fa2a4) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc0beca9064d8081d45c1) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test14c55f7cd295d89763ca) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test72a93f054d9296607dff) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf0ac164fe5c38cc36922) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test74efb7c560fd057d25ba) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test43adceaba606a7f5013f) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test94160894abf5f0650ec9) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testb77f1131af63dae91031) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test296aa575c385013e91f0) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test339bddce4b70064141c4) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test991a70285cf143adb7fe) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1b1ae70c1b5e7a1a2502) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test02e58fb30f5a5b3616ec) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testbdc3952445cad78094e2) {
  Emitter out;
  out << BeginSeq;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test5d24f2ab8e24cb71d6c9) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1ca2c58583cb7dd8a765) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test6086aee45faab48750ad) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testdac42de03b96b1207ec4) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test10d18ea5e198359e218b) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test56218e461d6be3a18500) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9acfd124b72471e34bbd) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test2a1c3780a4dfaa43646e) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test91e4c547fdab9e8b1c67) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test3d7e8318208742fe4358) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test2e4a92f93d5f9d8c5fed) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9abf5d48ef7c6f2ed8a0) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testc3428819fe7cfe88cf10) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test8007ba3728b0fdbb0cb8) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test6eedc1e3db4ceee9caf6) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testd892f2048c7066c74b7e) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test736430339c2a221b6d89) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test6d51f33adb9324f438d1) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << Comment("comment");
  out << "foo";
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test00d50067643ed73a3f7f) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << Comment("comment");
  out << EndSeq;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test5fc029e9d46151d31a80) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test328542a5a4b65371d2c6) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testf791a97db1c96e9f16c7) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginSeq;
  out << "foo";
  out << EndSeq;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test44ac18a00d604391a169) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste7ff269f6d95faa06abe) {
  Emitter out;
  out << Comment("comment");
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test8d056d159803415c2c85) {
  Emitter out;
  out << BeginSeq;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test47ef0ff3da945fda8680) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test1f981851e5a72a91614b) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test783be6c196784ca7ff30) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test217dcab50ef45ac6d344) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testec71f98bc646a34c9327) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << Comment("comment");
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test04595ac13c58c0740048) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << Comment("comment");
  out << "bar";
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test18d724e2ff0f869e9947) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << Comment("comment");
  out << EndMap;
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, test9f80798acafd4cfec0aa) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << Comment("comment");
  out << EndSeq;

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, testeabc051f6366e0275232) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

TEST_F(GenEmitterTest, teste33a98fb01ea45cc91dc) {
  Emitter out;
  out << BeginSeq;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << BeginMap;
  out << "foo";
  out << "bar";
  out << EndMap;
  out << EndSeq;
  out << Comment("comment");

  EXPECT_CALL(handler, OnDocumentStart(_));
  EXPECT_CALL(handler, OnSequenceStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnMapStart(_, "?", 0, _));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "foo"));
  EXPECT_CALL(handler, OnScalar(_, "?", 0, "bar"));
  EXPECT_CALL(handler, OnMapEnd());
  EXPECT_CALL(handler, OnSequenceEnd());
  EXPECT_CALL(handler, OnDocumentEnd());
  Parse(out.c_str());
}

}  // namespace
}  // namespace YAML
