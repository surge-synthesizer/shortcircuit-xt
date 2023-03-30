/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
 * transaction log.
 *
 * ShortcircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortcircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortcircuitXT is inspired by, and shares code with, the
 * commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#include "sfz_parse.h"

#define TAO_PEGTL_NAMESPACE scxt::sfz::pegtl
#include <iostream>
#include "tao/pegtl.hpp"
#include "tao/pegtl/contrib/parse_tree.hpp"
#include "tao/pegtl/contrib/trace.hpp"

namespace scxt::sfz_support
{

namespace pegtl = TAO_PEGTL_NAMESPACE;

struct SFZAccumulator
{
    SFZParser::document_t result;

    SFZParser::OpCode currentOpCode;

    void setOpCodeValue(float f) { currentOpCode.value = f; }
    void setOpCodeValue(const std::string &s)
    {
        if (s.empty())
            currentOpCode.value = true;
        else
            currentOpCode.value = s;
    }
    void setOpCodeName(const std::string &s) { currentOpCode.name = s; }

    void pushCurrentOpCode()
    {
        if (result.empty())
            throw std::logic_error("opcode before header");
        result.back().second.push_back(currentOpCode);
        currentOpCode = {};
    }
    void pushHeader(const SFZParser::Header &h) { result.push_back({h, {}}); }
};

namespace grammar
{
using namespace pegtl;
// CLang Format wants all these empty structs to be braced on newlines so turn it off for now
// clang-format off
struct expression;

struct sep : sor<ascii::space, eolf> {};

struct single_line_comment : seq<star<space>, TAO_PEGTL_STRING("//"), until<eolf>> {};
struct delimited_comment : seq<star<space>, TAO_PEGTL_STRING("/*"), until<TAO_PEGTL_STRING("*/")>> {};
// TODO multi-line-comment
struct comment : sor<single_line_comment, pad<delimited_comment, space>> {};


struct hdr_region : TAO_PEGTL_STRING("region") {};
struct hdr_group : TAO_PEGTL_STRING("group") {};
struct hdr_control : TAO_PEGTL_STRING("control") {};
struct hdr_global : TAO_PEGTL_STRING("global") {};
struct hdr_curve : TAO_PEGTL_STRING("curve") {};
struct hdr_effect : TAO_PEGTL_STRING("effect") {};
struct hdr_master : TAO_PEGTL_STRING("master") {};
struct hdr_midi : TAO_PEGTL_STRING("midi") {};
struct hdr_sample : TAO_PEGTL_STRING("sample") {};

struct hdr_known : sor<hdr_region, hdr_group, hdr_control, hdr_global, hdr_curve, hdr_effect, hdr_master, hdr_midi, hdr_sample> {};
struct hdr_unknown : seq<alpha> {};
struct hdr_name : sor<hdr_known, hdr_unknown> {};

struct header : seq< one<'<'>, hdr_name, one< '>'> > {};

// TODO - later add some custom opcode types as parsed types, especialy the onCC ones
struct opcode_generic : plus<sor<alnum, one<'_'>>> {};
struct opcode : sor<opcode_generic> {};

struct space_or_comment : sor<space, eof, comment> {};
struct opcode_numvalue : seq<opt<one<'-'>>, seq<plus<digit>, opt<one<'.'>>, star<digit>>> {};
struct opcode_strvalue : star< not_at<space_or_comment>, any> {};
struct opcode_anyvalue : sor<opcode_numvalue, opcode_strvalue> {};
struct opcode_value : seq<opcode_anyvalue, space_or_comment> {};
//struct opcode_value : sor<numvalue, strvalue> {};


struct opcode_assignment : seq<opcode, one<'='>, opcode_value> {};

struct expression : sor<pad<header, space>, pad<opcode_assignment, plus<space>>, pad<comment, space>> {};
struct document : seq<star<expression>, eof> {};
// clang-format on

template <typename Rule>
using selector = pegtl::parse_tree::selector<
    Rule, parse_tree::fold_one::on<>,
    parse_tree::store_content::on<
        // comment, single_line_comment, delimited_comment,
        header, hdr_region, hdr_group, hdr_control, hdr_global, hdr_curve, hdr_effect, hdr_master,
        hdr_midi, hdr_sample, hdr_unknown, opcode, opcode_generic,
        // opcode_value,
        opcode_strvalue, opcode_numvalue, opcode_assignment,
        //                          expression,
        document>>;

template <typename Rule> struct sfz_action : nothing<Rule>
{
};

#define HDR_HELPER(a)                                                                              \
    template <> struct sfz_action<hdr_##a>                                                         \
    {                                                                                              \
        template <typename ActionInput>                                                            \
        static void apply(const ActionInput &in, SFZAccumulator &out)                              \
        {                                                                                          \
            SFZParser::Header h;                                                                   \
            h.type = SFZParser::Header::Type::a;                                                   \
            h.name = in.string();                                                                  \
            out.pushHeader(h);                                                                     \
        }                                                                                          \
    };

HDR_HELPER(region);
HDR_HELPER(group);
HDR_HELPER(control);
HDR_HELPER(global);
HDR_HELPER(curve);
HDR_HELPER(effect);
HDR_HELPER(master);
HDR_HELPER(midi);
HDR_HELPER(sample);
HDR_HELPER(unknown);

template <> struct sfz_action<opcode_assignment>
{
    template <typename ActionInput> static void apply(const ActionInput &in, SFZAccumulator &out)
    {
        out.pushCurrentOpCode();
    }
};

template <> struct sfz_action<opcode>
{
    template <typename ActionInput> static void apply(const ActionInput &in, SFZAccumulator &out)
    {
        out.setOpCodeName(in.string());
    }
};

template <> struct sfz_action<opcode_numvalue>
{
    template <typename ActionInput> static void apply(const ActionInput &in, SFZAccumulator &out)
    {
        out.setOpCodeValue(atof(in.string().c_str()));
    }
};

template <> struct sfz_action<opcode_strvalue>
{
    template <typename ActionInput> static void apply(const ActionInput &in, SFZAccumulator &out)
    {
        out.setOpCodeValue(in.string());
    }
};

}; // namespace grammar

template <typename N> void printTree(std::ostream &os, const N &n, const std::string &pfx = "")
{
    os << pfx;
    if (!n.is_root())
        os << n.type;
    else
        os << "ROOT";
    if (n.has_content())
        os << "[Has content] " << n.string();

    os << std::endl;
    for (auto &c : n.children)
        printTree(os, *c, pfx + "|..");
}

SFZParser::document_t SFZParser::parse(const std::string &contents)
{

    try
    {
        /*
        pegtl::string_input<> trin(contents, "Provided Expression");
        pegtl::standard_trace<grammar::document>(trin);


        pegtl::string_input<> in(contents, "Provided Expression");
        auto root = pegtl::parse_tree::parse<grammar::document, grammar::selector>(in);
        if (root)
        {
            printTree(std::cout, *root);
        }
        else
        {
            std::cout << "No Root in Parse" << std::endl;
        }
*/
        pegtl::string_input<> pin(contents, "Provided Expression");
        SFZAccumulator a;
        pegtl::parse<grammar::document, grammar::sfz_action>(pin, a);
        return a.result;
    }
    catch (const pegtl::parse_error &e)
    {
        std::cout << e.what() << std::endl;
    }
    return {};
}
} // namespace scxt::sfz_support