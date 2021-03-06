#include "thirdparty/microhcl/hcl.hpp"
#include "thirdparty/microhil/hil.hpp"

#include "i18n.hpp"
#include <memory>
#include <fstream>
#include "cat.hpp"
#include "elona.hpp"
#include "filesystem.hpp"
#include "random.hpp"
#include "variables.hpp"



namespace elona
{
namespace i18n
{

i18n::store s;

void store::init(fs::path path)
{
    storage.clear();
    for (const auto& entry : filesystem::dir_entries{
             path, filesystem::dir_entries::type::file})
    {
        std::ifstream ifs(filesystem::make_preferred_path_in_utf8(entry.path()));
        load(ifs, entry.path().string());
    }
}

void store::load(std::istream& is, const std::string& hcl_file)
{
    hcl::ParseResult parseResult = hcl::parse(is);

    if (!parseResult.valid()) {
        std::cerr << parseResult.errorReason << std::endl;
        throw i18n_error(hcl_file, parseResult.errorReason);
    }

    const hcl::Value& value = parseResult.value;

    if (!value.is<hcl::Object>() || !value.has("locale"))
    {
        throw i18n_error(hcl_file, "\"locale\" object not found");
    }

    const hcl::Value locale = value["locale"];

    visit_object(locale.as<hcl::Object>(), "core.locale", hcl_file);
}

void store::visit_object(const hcl::Object& object,
                              const std::string& current_key,
                              const std::string& hcl_file)
{
    for (const auto& pair : object)
    {
        visit(pair.second, current_key + "." + pair.first, hcl_file);
    }
}

void store::visit(const hcl::Value& value,
                       const std::string& current_key,
                       const std::string& hcl_file)
{
    if (value.is<std::string>())
    {
        std::stringstream ss(value.as<std::string>());

        hil::ParseResult p = hil::parse(ss);
        // TODO validate ident names?
        if (!p.valid())
        {
            throw i18n_error(hcl_file, "HIL parse error: " + p.errorReason);
        }

        storage.emplace(current_key, std::move(p.context));
    }
    else if (value.is<hcl::Object>())
    {
        visit_object(value.as<hcl::Object>(), current_key, hcl_file);
    }
}


// TODO
// "${your(_1)}${get(_2, core.locale.ability, name)} skill increases."


#define ELONA_DEFINE_I18N_BUILTIN(func_name, return_value) \
    if(func.name == func_name) \
    { \
        return return_value; \
    }

std::string format_builtins_argless(const hil::FunctionCall& func)
{
    ELONA_DEFINE_I18N_BUILTIN("you", name(0));

    return "<unknown function (" + func.name + ")>";
}

inline std::string builtin_s(const hil::FunctionCall& func, int chara_index)
{
    bool needs_e = false;
    if(func.args.size() > 1)
    {
        needs_e = func.args[1].as<bool>();
    }
    return _s(chara_index, needs_e);
}

std::string format_builtins_bool(const hil::FunctionCall& func, bool value)
{
    ELONA_DEFINE_I18N_BUILTIN("s", builtin_s(func, value ? 0 : 1));
    ELONA_DEFINE_I18N_BUILTIN("is", is(value ? 0 : 1));

    return "<unknown function (" + func.name + ")>";
}

std::string format_builtins_character(const hil::FunctionCall& func, const character& chara)
{
    ELONA_DEFINE_I18N_BUILTIN("name", name(chara.index));
    ELONA_DEFINE_I18N_BUILTIN("basename", cdatan(0, chara.index));
    ELONA_DEFINE_I18N_BUILTIN("he2", he(chara.index, 1));
    ELONA_DEFINE_I18N_BUILTIN("his2", his(chara.index, 1));
    ELONA_DEFINE_I18N_BUILTIN("him2", him(chara.index, 1));

    // English only
    ELONA_DEFINE_I18N_BUILTIN("he", he(chara.index));
    ELONA_DEFINE_I18N_BUILTIN("his", his(chara.index));
    ELONA_DEFINE_I18N_BUILTIN("him", him(chara.index));
    ELONA_DEFINE_I18N_BUILTIN("s", builtin_s(func, chara.index));
    ELONA_DEFINE_I18N_BUILTIN("is", is(chara.index));
    ELONA_DEFINE_I18N_BUILTIN("have", have(chara.index));
    ELONA_DEFINE_I18N_BUILTIN("himself", yourself(chara.index));
    ELONA_DEFINE_I18N_BUILTIN("his_owned", your(chara.index));
    ELONA_DEFINE_I18N_BUILTIN("name_nojob", sncnv(cdatan(0, chara.index)));

    // Japanese only
    ELONA_DEFINE_I18N_BUILTIN("kare_wa", npcn(chara.index));
    //ELONA_DEFINE_I18N_BUILTIN("yoro", _yoro(chara, func.args.at(1));
    //ELONA_DEFINE_I18N_BUILTIN("dozo", _dozo(chara, func.args.at(1));
    //ELONA_DEFINE_I18N_BUILTIN("thanks", _thanks(chara, func.args.at(1));
    //ELONA_DEFINE_I18N_BUILTIN("rob", _rob(chara, func.args.at(1));
    //ELONA_DEFINE_I18N_BUILTIN("ka", _ka(chara, func.args.at(1));
    //ELONA_DEFINE_I18N_BUILTIN("da", _da(chara, func.args.at(1));
    //ELONA_DEFINE_I18N_BUILTIN("nda", _nda(chara, func.args.at(1));
    //ELONA_DEFINE_I18N_BUILTIN("noka", _noka(chara, func.args.at(1));
    //ELONA_DEFINE_I18N_BUILTIN("kana", _kana(chara, func.args.at(1));
    //ELONA_DEFINE_I18N_BUILTIN("kimi", _kimi(chara, func.args.at(1));
    //ELONA_DEFINE_I18N_BUILTIN("ru", _ru(chara, func.args.at(1));
    //ELONA_DEFINE_I18N_BUILTIN("tanomu", _tanomu(chara, func.args.at(1));
    //ELONA_DEFINE_I18N_BUILTIN("ore", _ore(chara, func.args.at(1));
    //ELONA_DEFINE_I18N_BUILTIN("ga", _ga(chara, func.args.at(1));
    //ELONA_DEFINE_I18N_BUILTIN("dana", _dana(chara, func.args.at(1));
    //ELONA_DEFINE_I18N_BUILTIN("kure", _kure(chara, func.args.at(1));
    //ELONA_DEFINE_I18N_BUILTIN("daro", _daro(chara, func.args.at(1));
    //ELONA_DEFINE_I18N_BUILTIN("yo", _yo(chara, func.args.at(1));
    //ELONA_DEFINE_I18N_BUILTIN("aru", _aru(chara, func.args.at(1));
    //ELONA_DEFINE_I18N_BUILTIN("u", _u(chara, func.args.at(1));
    //ELONA_DEFINE_I18N_BUILTIN("na", _na(chara, func.args.at(1));
    //ELONA_DEFINE_I18N_BUILTIN("ta", _ta(chara, func.args.at(1));

    return "<unknown function (" + func.name + ")>";
}

std::string format_builtins_item(const hil::FunctionCall& func, const item& item)
{
    ELONA_DEFINE_I18N_BUILTIN("name", itemname(item.index));
    ELONA_DEFINE_I18N_BUILTIN("basename", ioriginalnameref(item.id));

    // English only
    ELONA_DEFINE_I18N_BUILTIN("is", is2(item.number));
    ELONA_DEFINE_I18N_BUILTIN("s", _s2(item.number));
    ELONA_DEFINE_I18N_BUILTIN("does", does(item.number));

    return "<unknown function (" + func.name + ")>";
}

#undef ELONA_DEFINE_I18N_BUILTIN


void load(const std::string& language)
{
    for (auto&& entry : filesystem::dir_entries{
             filesystem::path(u8"lang") / language,
             filesystem::dir_entries::type::file,
         })
    {
        cat::global.load(entry.path());
    }
}


std::string _(
    const std::string& key_head,
    const std::vector<std::string>& key_tail)
{
    lua_getglobal(cat::global.ptr(), key_head.c_str());
    int pop_count = 1;
    for (const auto& k : key_tail)
    {
        lua_getfield(cat::global.ptr(), -1, k.c_str());
        ++pop_count;
    }
    if (lua_istable(cat::global.ptr(), -1))
    {
        lua_rawgeti(
            cat::global.ptr(), -1, rnd(lua_rawlen(cat::global.ptr(), -1)) + 1);
        ++pop_count;
    }
    const char* ret = lua_tostring(cat::global.ptr(), -1);
    lua_pop(cat::global.ptr(), pop_count);
    return ret ? ret : "";
}

std::string space_if_needed()
{
    if (jp)
    {
        return "";
    }
    else
    {
        return u8" ";
    }
}



} // namespace i18n
} // namespace elona
