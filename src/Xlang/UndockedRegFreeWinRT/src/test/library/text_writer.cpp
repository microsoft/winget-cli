#include "pch.h"
#include "meta_reader.h"
#include "text_writer.h"

namespace
{
    struct writer : xlang::text::writer_base<writer>
    {
    };
}

TEST_CASE("writer")
{
    writer w;
    w.write(" % ^% % post", 123, "String");
    w.swap();
    w.write("pre");

    REQUIRE(w.flush_to_string() == "pre 123 % String post");
}
