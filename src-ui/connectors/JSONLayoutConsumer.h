/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
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

#ifndef SCXT_SRC_UI_CONNECTORS_JSONLAYOUTCONSUMER_H
#define SCXT_SRC_UI_CONNECTORS_JSONLAYOUTCONSUMER_H

#include <string>
#include <vector>

#include "tao/json/binary_view.hpp"
#include "tao/json/events/from_string.hpp"

namespace scxt::ui::connectors
{
struct JSONLayoutLibrary
{
    static std::string jsonForComponent(const std::string &nm);
};

struct JSONLayoutConsumer
{
    static constexpr bool debugStateMachine{false};
#define DLOG(x)                                                                                    \
    if (debugStateMachine)                                                                         \
        SCLOG(pfx << "[" << state << "] " << __func__ << x);
#define DMARK() DLOG(" ")

    enum State
    {
        NOTHING,
        TOP_LEVEL,
        IN_DEFAULTS,
        COLLECT_DEFAULTS,
        IN_COMPONENTS,
        IN_COMPONENTS_ARRAY,
        COLLECT_COMPONENT,
        EXPECTING_INDEX,
        COLLECT_COMPONENT_COORDS
    } state{NOTHING};

    std::string currKey{};
    std::string currValString{};
    int64_t currValInt{};
    enum currValType
    {
        STRING,
        I64
    } currValType{STRING};

    struct ComponentHelper
    {
        std::array<float, 4> coords{};
        int index{-1};
        std::unordered_map<std::string, std::string> map;
        std::unordered_map<std::string, int64_t> intMap;
    } currentComponent, defaultComponent;
    int currentCoord{0};

    std::vector<ComponentHelper> result;

    std::string pfx{""};
    void push() { pfx += "|--"; }
    void pop() { pfx = pfx.substr(pfx.size() - 3); }

    void null() {}
    void boolean(const bool) {}
    void number(const double n)
    {
        if (state == COLLECT_COMPONENT_COORDS)
        {
            assert(currentCoord < 4);
            assert(currentCoord >= 0);
            currentComponent.coords[currentCoord] = n;
            currentCoord++;
        }
    }
    void number(const std::int64_t n)
    {
        if (state == COLLECT_COMPONENT)
        {
            currValInt = n;
            currValType = I64;
        }
    }
    void number(const std::uint64_t n)
    {
        if (state == EXPECTING_INDEX)
        {
            currentComponent.index = n;
        }
        else if (state == COLLECT_COMPONENT_COORDS)
        {
            assert(currentCoord < 4);
            assert(currentCoord >= 0);
            currentComponent.coords[currentCoord] = n;
            currentCoord++;
        }
        else if (state == COLLECT_COMPONENT)
        {
            currValInt = (int64_t)n;
            currValType = I64;
        }
    }
    void string(const std::string_view val)
    {
        currValString = "";
        if (state == COLLECT_DEFAULTS || state == COLLECT_COMPONENT)
        {
            currValString = val;
            currValType = STRING;
        }
    }
    void binary(const tao::binary_view) {}
    void begin_array(const std::size_t = 0)
    {
        switch (state)
        {
        case IN_COMPONENTS:
            state = IN_COMPONENTS_ARRAY;
            break;
        case COLLECT_COMPONENT_COORDS:
            break;
        default:
            DLOG("Warning: Array open unexpected");
            break;
        }

        DMARK();
        push();
    }
    void element() {}
    void end_array(const std::size_t = 0)
    {
        switch (state)
        {
        case IN_COMPONENTS_ARRAY:
            state = IN_COMPONENTS;
            break;
        case COLLECT_COMPONENT_COORDS:
            break;
        default:
            DLOG("Warning: Unexpected end array");
            break;
        }
        DMARK();
        pop();
    }
    void begin_object(const std::size_t = 0)
    {
        switch (state)
        {
        case NOTHING:
            state = TOP_LEVEL;
            break;
        case IN_DEFAULTS:
            state = COLLECT_DEFAULTS;
            break;
        case IN_COMPONENTS_ARRAY:
            state = COLLECT_COMPONENT;
            currentComponent = defaultComponent;
            break;
        default:
            DLOG("Warning: Unexpected begin object");
            break;
        }
        DMARK();
        push();
    }
    void key(const std::string_view s)
    {
        switch (state)
        {
        case TOP_LEVEL:
            if (s == "defaults")
            {
                state = IN_DEFAULTS;
            }
            else if (s == "components")
            {
                state = IN_COMPONENTS;
            }
            else
            {
                throw std::runtime_error(std::string() + "Unexpected top level key " +
                                         std::string(s));
            }
            break;
        case COLLECT_DEFAULTS:
            currKey = s;
            break;
        case COLLECT_COMPONENT:
            currKey = s;
            if (s == "coordinates")
            {
                state = COLLECT_COMPONENT_COORDS;
                currentCoord = 0;
            }
            if (s == "index")
            {
                state = EXPECTING_INDEX;
            }
            break;
        default:
            DLOG("Warning: Unexpected key " << s);
            break;
        }
        DMARK();
    }
    void member()
    {
        switch (state)
        {
        case COLLECT_DEFAULTS:
        case COLLECT_COMPONENT:
        {
            auto tgt = &defaultComponent;
            if (state == COLLECT_COMPONENT)
                tgt = &currentComponent;
            switch (currValType)
            {
            case STRING:
                tgt->map[currKey] = currValString;
                break;
            case I64:
                tgt->intMap[currKey] = currValInt;
                break;
            }
        }
        break;
        case COLLECT_COMPONENT_COORDS:
            state = COLLECT_COMPONENT;
            break;
        case EXPECTING_INDEX:
            assert(currentComponent.index >= 0);
            state = COLLECT_COMPONENT;
            break;
        case IN_DEFAULTS:
        {
            state = TOP_LEVEL;
        }
        break;
        case IN_COMPONENTS:
        {
            state = TOP_LEVEL;
        }
        break;
        default:
            DLOG("Warning: Unexpected member call");
            break;
        }
        DMARK();
    }
    void end_object(const std::size_t = 0)
    {
        switch (state)
        {
        case COLLECT_DEFAULTS:
            state = IN_DEFAULTS;
            break;
        case COLLECT_COMPONENT:
        {
            state = IN_COMPONENTS_ARRAY;
            result.push_back(currentComponent);
        }
        break;
        case TOP_LEVEL:
            state = NOTHING;
            break;
        default:
            DLOG("Warning: Unexpected end of object");
            break;
        }
        DMARK();
        pop();
    }

#undef DLOG
#undef DMARK
};
} // namespace scxt::ui::connectors

#endif // SHORTCIRCUITXT_JSONLAYOUTCONSUMER_H
