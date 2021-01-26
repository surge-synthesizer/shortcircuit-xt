/*
** ShortCircuit3 is Free and Open Source Software
**
** ShortCircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2021 by various individuals as described by the Git transaction log
**
** All source at: https://github.com/surge-synthesizer/surge.git
**
** ShortCircuit was a commercial product from 2004-2018, with Copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made ShortCircuit
** open source in December 2020.
*/

#if WINDOWS
#include <windows.h>
#endif

#include "globals.h"
#include "sampler.h"

#include "configuration.h"
#include "synthesis/modmatrix.h"
#include "util/unitconversion.h"

#include <map>
#include <string>
#include <algorithm>

using std::max;

static int keyname_to_keynumber(const char *name) // using C4 == 60
{
    int key = 0;

    if (name[0] < 64)
    {
        return atoi(name); // first letter is number, must be in numeric format then
    }

    char letter = tolower(name[0]);

    switch (letter)
    {
    case 'c':
        key = 0;
        break;
    case 'd':
        key = 2;
        break;
    case 'e':
        key = 4;
        break;
    case 'f':
        key = 5;
        break;
    case 'g':
        key = 7;
        break;
    case 'a':
        key = 9;
        break;
    case 'b':
    case 'h':
        key = 11;
        break;
    }

    const char *c = name + 1;
    if (*c == '#')
        key++;
    else if (*c == 'b' || *c == 'B')
        key--;
    else
        c--;
    c++;

    bool negative = (*c == '-');
    if (negative)
        c++;
    int octave = (*c - 48);
    if (negative)
        octave = -octave;
    octave = max(0, octave + 1);

    return 12 * octave + key;
}

/**
 * Useful for debugging unsupported SFZ opcodes for a given zone.
 */
inline void dump_opcodes(sampler *s, std::map<std::string, std::string> &working_opcodes)
{
    std::stringstream ss;
    ss << "Opcode dump for incomplete SFZ zone created: {" << std::endl;
    for (auto [k, v] : working_opcodes)
        ss << '\t' << k << ": " << v << std::endl;
    ss << '}';
    LOGDEBUG(s->mLogger) << ss.str();
}

/**
 * Take the current SFZ <region> opcodes and create a sample zone out of it.
 * This assumes <group> opcodes have already been integrated into the std::map.
 */
bool create_sfz_zone(sampler *s, std::map<std::string, std::string> &sfz_zone_opcodes,
                     const fs::path &path, const char &channel)
{
    if (sfz_zone_opcodes.empty())
        return false;

    // Create zone (assuming region with group opcodes already integrated)
    uint8_t num_opcodes_processed = 0;
    int z_id;

    // Transient zone values (Crossfade)
    int xfin_lokey = -1, xfin_hikey = -1, xfout_lokey = -1, xfout_hikey = -1;
    int xfin_lovel = -1, xfin_hivel = -1, xfout_lovel = -1, xfout_hivel = -1;

    // First check if a zone CAN be created - does it even have a sample?
    if (sfz_zone_opcodes.find("sample") == sfz_zone_opcodes.end())
    {
        LOGERROR(s->mLogger) << "Zone not created due to missing SFZ sample opcode.";
        return false;
    }
    
    std::stringstream ss;
    sample_zone *z;

    // Normalise sample path first before creating a zone to load it.
    // TODO: Pseudo-samples eg: *saw *sine *triangle etc. TBI
    ss << path_to_string(path) << static_cast<char>(fs::path::preferred_separator)
       << sfz_zone_opcodes["sample"];
    auto sample_path = fs::canonical(string_to_path(ss.str()));

    if (!fs::exists(sample_path))
    {
        LOGERROR(s->mLogger) << "Zone not created due to invalid sample path: " << sample_path;
        return false;
    }

    if (s->add_zone(sample_path, &z_id, channel, false))
    {
        z = &s->zones[z_id];

        // Apply zone defaults where opcodes are missing
        // (behaviour matches Polyphone and some other sfz players)
        bool no_key_macro = (sfz_zone_opcodes.find("key") == sfz_zone_opcodes.end());
        bool no_low_key = (sfz_zone_opcodes.find("lokey") == sfz_zone_opcodes.end());
        bool no_high_key = (sfz_zone_opcodes.find("hikey") == sfz_zone_opcodes.end());
        bool no_keycenter = (sfz_zone_opcodes.find("pitch_keycenter") == sfz_zone_opcodes.end());

        if (no_key_macro)
        {
            if (no_keycenter)
                sfz_zone_opcodes.insert({"pitch_keycenter", "60"});
            if (no_low_key)
                sfz_zone_opcodes.insert({"lokey", "0"});
            if (no_high_key)
                sfz_zone_opcodes.insert({"hikey", "127"});
        }
    }
    else
    {
        LOGERROR(s->mLogger) << "Failed to create zone for SFZ sample: " << sample_path;
        return false;
    }

    // Apply each opcode to the newly created zone.
    for (auto [k, v] : sfz_zone_opcodes)
    {
        const char *opcode = k.c_str(), *val = v.c_str();

        if (stricmp(opcode, "sample") == 0)
        {
            ++num_opcodes_processed;
            // already used it above.
        }
        else if (stricmp(opcode, "key") == 0)
        {
            z->key_root = keyname_to_keynumber(val);
            z->key_low = keyname_to_keynumber(val);
            z->key_high = keyname_to_keynumber(val);
            ++num_opcodes_processed;
        }
        else if (stricmp(opcode, "lokey") == 0)
        {
            z->key_low = keyname_to_keynumber(val);
            ++num_opcodes_processed;
        }
        else if (stricmp(opcode, "hikey") == 0)
        {
            z->key_high = keyname_to_keynumber(val);
            ++num_opcodes_processed;
        }
        else if (stricmp(opcode, "pitch_keycenter") == 0)
        {
            z->key_root = keyname_to_keynumber(val);
            ++num_opcodes_processed;
        }
        else if (stricmp(opcode, "lovel") == 0)
        {
            z->velocity_low = atol(val);
            ++num_opcodes_processed;
        }
        else if (stricmp(opcode, "hivel") == 0)
        {
            z->velocity_high = atol(val);
            ++num_opcodes_processed;
        }
        else if (stricmp(opcode, "pitch_keytrack") == 0)
        {
            z->keytrack = atof(val);
            ++num_opcodes_processed;
        }
        else if (stricmp(opcode, "transpose") == 0)
        {
            z->transpose = atoi(val);
            ++num_opcodes_processed;
        }
        else if (stricmp(opcode, "tune") == 0)
        {
            z->finetune = (float)0.01f * atoi(val);
            ++num_opcodes_processed;
        }
        else if (stricmp(opcode, "volume") == 0)
        {
            z->aux[0].level = atof(val);
            ++num_opcodes_processed;
        }
        else if (stricmp(opcode, "pan") == 0)
        {
            z->aux[0].balance = (float)0.01f * atof(val);
            ++num_opcodes_processed;
        }
        else if (stricmp(opcode, "ampeg_attack") == 0)
        {
            z->AEG.attack = log2(atof(val));
            ++num_opcodes_processed;
        }
        else if (stricmp(opcode, "ampeg_hold") == 0)
        {
            z->AEG.hold = log2(atof(val));
            ++num_opcodes_processed;
        }
        else if (stricmp(opcode, "ampeg_decay") == 0)
        {
            z->AEG.decay = log2(atof(val));
            ++num_opcodes_processed;
        }
        else if (stricmp(opcode, "ampeg_release") == 0)
        {
            z->AEG.release = log2(atof(val));
            ++num_opcodes_processed;
        }
        else if (stricmp(opcode, "ampeg_sustain") == 0)
        {
            z->AEG.sustain = 0.01f * atof(val);
            ++num_opcodes_processed;
        }
        else if (stricmp(opcode, "loop_mode") == 0)
        {
            if (stricmp(val, "loop_continuous") == 0)
                z->playmode = pm_forward_loop;
            else if (stricmp(val, "loop_sustain") == 0)
                z->playmode = pm_forward_loop_until_release;
            else if (stricmp(val, "one_shot") == 0)
                z->playmode = pm_forward_shot;
            else // "no_loop" or invalid values will use default
                z->playmode = pm_forward;
            ++num_opcodes_processed;
        }
        else if (stricmp(opcode, "amp_velcurve") == 0)
        {
            // TODO verify alignment of SFZ curvature to SC values
            z->velsense = atof(val);
            ++num_opcodes_processed;
        }
        else if (stricmp(opcode, "seq_length") == 0)
        {
            // TBI: need note groups properly implemented/testable first.
            LOGWARNING(s->mLogger) << "SFZ 1.0 seq_length not yet implemnted";
        }
        else if (stricmp(opcode, "seq_position") == 0)
        {
            // TBI: need note groups properly implemented/testable first.
            LOGWARNING(s->mLogger) << "SFZ 1.0 seq_position not yet implemnted";
        }
        else
        {
            // TODO check if crossfading implementation is correct
            if (stricmp(opcode, "xfin_lokey") == 0)
                xfin_lokey = keyname_to_keynumber(val);
            else if (stricmp(opcode, "xfin_hikey") == 0)
                xfin_hikey = keyname_to_keynumber(val);
            else if (stricmp(opcode, "xfout_lokey") == 0)
                xfout_lokey = keyname_to_keynumber(val);
            else if (stricmp(opcode, "xfout_hikey") == 0)
                xfout_hikey = keyname_to_keynumber(val);
            else if (stricmp(opcode, "xfin_lovel") == 0)
                xfin_lovel = atoi(val);
            else if (stricmp(opcode, "xfin_hivel") == 0)
                xfin_hivel = atoi(val);
            else if (stricmp(opcode, "xfout_lovel") == 0)
                xfout_lovel = atoi(val);
            else if (stricmp(opcode, "xfout_hivel") == 0)
                xfout_hivel = atoi(val);

            // TODO other opcodes not yet mapped:
            // see https://sfzformat.com/legacy/
        }
    }

    // Crossfade handler has to be done per-zone (to be order independent).
    if (xfin_lokey >= 0)
    {
        z->key_low = xfin_lokey + 1;
        ++num_opcodes_processed;
    }
    if (xfin_hikey >= 0)
    {
        z->key_low_fade = xfin_hikey - z->key_low;
        ++num_opcodes_processed;
    }
    if (xfout_hikey >= 0)
    {
        z->key_high = xfout_lokey + 1;
        ++num_opcodes_processed;
    }
    if (xfout_lokey >= 0)
    {
        z->key_high_fade = xfout_hikey - z->key_high;
        ++num_opcodes_processed;
    }
    if (xfin_lovel >= 0)
    {
        z->velocity_low = xfin_lovel + 1;
        ++num_opcodes_processed;
    }
    if (xfin_hivel >= 0)
    {
        z->velocity_low_fade = xfin_hivel - z->velocity_low;
        ++num_opcodes_processed;
    }
    if (xfout_hivel >= 0)
    {
        z->velocity_high = xfout_lovel + 1;
        ++num_opcodes_processed;
    }
    if (xfout_lovel >= 0)
    {
        z->velocity_high_fade = xfout_hivel - z->velocity_high;
        ++num_opcodes_processed;
    }

    s->update_zone_switches(z_id);

    if (num_opcodes_processed < sfz_zone_opcodes.size())
    {
        LOGWARNING(s->mLogger) << "Zone creation did not process all SFZ opcodes.";
        return false;
    }

    return true;
}

/**
 * If next data is typable, read in all opcudes until the next sfz <tag> is encountered
 * or otherwise reached the end.
 */
void parse_opcodes(sampler *s, const char *&r, const char *data_end, const fs::path &path,
                   std::map<std::string, std::string> &working_opcodes)
{
    if (r == nullptr || r >= data_end)
    {
        // TODO may need to refactor these loader functions rather than passing the mLogger object.
        LOGWARNING(s->mLogger) << "SFZ opcode parsing appears to have reached an unstable state.";
        return;
    }

    // Find the next tag start to mark end of processing opcodes for the current zone.
    const char *working_data_end = r;
    char buf[256];
    int copy_size;

    while (working_data_end < data_end && *working_data_end != '<')
        ++working_data_end;

    // Read all opcodes
    while (r < working_data_end)
    {
        // comment, skip ahead until the rest of the line
        if (*r == '/')
        {
            while (r < working_data_end && *r != '\n' && *r != '\r')
                r++;
            continue;
        }
        
        // Ignore whitespace, CRLF and control characters when looking for the start of an opcode.
        if (*r <= 32 || *r >= 127)
        {
            ++r;
            continue;
        }

        // Now look for the = operator (sfz spec says no spaces between opcode and value, but we'll
        // pretend they're not there in this case)
        const char *v = r;
        bool opcode_found = false;
        ++v;

        while (v < working_data_end)
        {
            // TODO really should use a regex here to only accept [\s0-9A-Za-z_]+ opcode labels
            // Annoyingly, some SFZ players allow incorrect opcodes such as "ampeg attack"
            // to work when they're not meant to... so white-space will be normalised later...
            if (*v != ' ' && *v < '0' && *v > '9' && *v < 'A' && *v > 'Z' && *v < 'a' && *v > 'z')
            {
                // If opcode name is not valid text, log invalid string and find the next one.
                copy_size = (v - r);
                strncpy(buf, r, copy_size);
                buf[copy_size] = '\0';
                LOGERROR(s->mLogger) << "Invalid SFZ opcode found: " << buf;
                r = v; // scan forward for the next SFZ opcode
                break;
            }

            if (*v == '=' && v < working_data_end)
            {
                opcode_found = true;
                break;
            }
            ++v;
        }

        if (opcode_found)
        {
            // Retain opcode name, normalise whitespace to _ and lowercase.
            copy_size = (v - r);
            for (int i = 0; i < copy_size; ++i)
            {
                if (r[i] <= ' ')
                    buf[i] = '_';
                else
                    buf[i] = tolower(r[i]);
            }
            buf[copy_size] = '\0';
            ++v;

            std::string opcode(buf), value;

            // Now obtain the value.
            const char *w = v;
            bool is_sample = false, parsed = false;
            bool is_quoted = false, quote_ended = false;

            // All opcode=value pairs can be space-delimited or across multiple lines, however due
            // care required for sample paths containing spaces. Note these are always relative to
            // the SFZ file location.
            // Valid cases:
            //   - Paths with no quotes around path value will assume entire line
            //     remainder until '.' is encountered in the path string (TODO can improve)
            //   - Paths with quotes will end at the next corresponding quote provided
            //     it is on the same line.
            if (stricmp(opcode.c_str(), "sample") == 0)
            {
                is_sample = true;

                // Note: other SFZ players eg: Polyphone, may not support quoted path strings.
                if (*w == '"')
                    is_quoted = true;

                ++w;
            }

            while (w < working_data_end && !parsed)
            {
                if (is_sample)
                {
                    if (is_quoted && !quote_ended)
                    {
                        // Where paths are quoted, find the corresponding end quote
                        // unless it's CRLF, which then we assume that's the value end.
                        if (*w != '"')
                        {
                            ++w;
                            if (*w == '\r' || *w == '\n')
                                is_sample = false;
                            else
                                continue;
                        }
                        else
                            quote_ended = true;
                    }
                    else if (!is_quoted)
                    {
                        copy_size = (w - v);
                        strncpy(buf, v, copy_size);
                        buf[copy_size] = '\0';

                        std::stringstream ss;
                        ss << path_to_string(path)
                           << static_cast<char>(fs::path::preferred_separator) << buf;
                        fs::path test_path = string_to_path(ss.str());
                        
                        if (fs::exists(test_path) && !fs::is_directory(test_path))
                            is_sample = false;
                    }
                }

                // Condition to parse opcode value - have we got everything by this point?
                if ((is_sample && *w < 32) || *w >= 127 || (!is_sample && *w <= 32))
                {
                    if (is_quoted && quote_ended)
                    {
                        --w;
                        ++v;
                    }
                    
                    copy_size = (w - v);
                    strncpy(buf, v, copy_size);
                    buf[copy_size] = '\0';
                    value = buf;

                    if (is_quoted)
                    {
                        if (quote_ended)
                        {
                            ++w;
                            --v;
                        }
                        else
                        {
                            LOGWARNING(s->mLogger)
                                << "Unterminated quote value found: " << opcode << "=" << value;
                        }
                    }

                    parsed = (!opcode.empty() && !value.empty());
                }
                ++w;
            }

            // Once here, if parsing had failed then log an error.
            if (!parsed)
            {
                LOGERROR(s->mLogger)
                    << "Unexpected SFZ opcode-value pair: " << opcode << "=" << value;
            }
            else
            {
                // Save the SFZ opcode=value pair
                working_opcodes.insert({opcode, value});
            }
            
            // move forward to the next opcode scan.
            r = w;
        }
        else
        {
            // Nothing to parse from this point.
            r = v;
        }
    }
}

/**
 * Entrypoint for SFZ loading.
 *
 * Assumptions (JN):
 * - Currently: Single-instrument SFZs for now. Multi-instrument loading support will come later.
 * - Only supporting SFZ 1.0 opcodes to start with... Can expand to 2.0 and ARIA in future.
 * - No Unicode or multilingual text (eg: umlauts, cyrillic etc.)
 * - ASCII-formatted SFZ data only.
 * - Apply default values where opcodes are unspecified (see behaviour of other existing
 *   SFZ players for guidance)
 * - Opcodes are procedurally parsed top-down with some nuance on group opcodes applied to regions.
 *
 * Rewrite SFZ parser... pseudocode
 *   1. (SFZ v1) Load group-region pairs.  (only create zones per group).
 *      a. Collect opcodes for <group>
 *      b. Collect opcodes for <region> until no more regions (eg: EOF or next <group>
 *      c. Create sample zones based on parsed groups, noting that:
 *          i. If no key ranges are specified, assume keys 0-127
 *         ii. If no vel ranges are specified, assume vels 0-127
 *        iii. If no ampeg settings, assume default ADSR.
 *         iv. group opcodes apply to all regions after it and before the next group.
 *             then region opcodes take precedence for each zone generated.
 *          v. (TODO) If using *sine *saw etc. possibly pre-load inbuilt sample in its place?
 *             (used for synthesized sfz presets)
 *      d. Repeat until there are no more groups/region tags.
 * TODO:
 *   2. (SFZ v2) Apply additional tags and follow similar pattern as 1.
 *      a. Pre-processor handling for #include etc.
 *   3. (ARIA) as above.
 */
bool sampler::load_sfz(const char *data, size_t datasize, const fs::path &path, int *new_g,
                       char channel)
{
    const char *r = data, *data_end = (data + datasize);
    std::map<std::string, std::string> global_opcodes, cur_group_opcodes, cur_region_opcodes,
        *working_opcodes = &cur_region_opcodes;

    // apply groups, then regions containing group-defined opcodes as relevant.
    // TODO do we need to work out how to re-use already loaded samples for multiple zones from
    // here?

    while (r < data_end)
    {
        if (*r == '<') // global -> group -> region
        {
            if (strnicmp(r, "<global>", 8) == 0)
            {
                // SFZ v2. There should only be one but...yeah.
                global_opcodes.clear();
                working_opcodes = &global_opcodes;
                r += 8;
            }
            else if (strnicmp(r, "<group>", 7) == 0)
            {
                // TODO: as SCG/SCM zone groups from SC1.1.2 are not implemented in SC2/3, for now
                // these act as 'pre-fills' to regions parsed to zones assigned to a single sample.
                // Eventually some group settings could apply similarly to SC1.1.2

                if (!cur_group_opcodes.empty())
                    cur_group_opcodes.clear();

                // At this point, all opcodes for a <region>/<group> would have been accumulated and
                // so should be processed.
                if (!cur_region_opcodes.empty())
                {
                    if (!create_sfz_zone(this, cur_region_opcodes, path, channel))
                        dump_opcodes(this, cur_region_opcodes);
                    cur_region_opcodes.clear();
                }

                // Where present, accumulate <global> opcodes.
                for (auto const &[opcode, val] : global_opcodes)
                    cur_region_opcodes.insert({opcode, val});

                working_opcodes = &cur_group_opcodes;
                r += 7;
            }
            else if (strnicmp(r, "<region>", 8) == 0)
            {
                if (!cur_region_opcodes.empty())
                {
                    if (!create_sfz_zone(this, cur_region_opcodes, path, channel))
                        dump_opcodes(this, cur_region_opcodes);
                    cur_region_opcodes.clear();
                }

                // Collect opcodes from last group read. If empty, ensure <global> is read.
                if (!cur_group_opcodes.empty())
                    for (auto const &[opcode, val] : cur_group_opcodes)
                        cur_region_opcodes.insert({opcode, val});
                else
                    for (auto const &[opcode, val] : global_opcodes)
                        cur_region_opcodes.insert({opcode, val});

                working_opcodes = &cur_region_opcodes;
                r += 8;
            }
            else
            {
                // Unknown tag, ignore
                while (r < data_end && *r != '>')
                {
                    r++;
                }
                r++;
            }
        }
        else if (*r == '/') // comment, skip ahead until the rest of the line
        {
            while (r < data_end && *r != '\n' && *r != '\r')
                r++;
        }
        else if (*r <= 32 || *r >= 127) // ignore whitespace/CR/LF, control characters
        {
            r++;
        }
        else
        {
            parse_opcodes(this, r, data_end, path, *working_opcodes);
        }
    }

    // If a region has not been processed, add it.
    if (!cur_region_opcodes.empty())
    {
        if (!create_sfz_zone(this, cur_region_opcodes, path, channel))
            dump_opcodes(this, cur_region_opcodes);
    }

    return true;
}
