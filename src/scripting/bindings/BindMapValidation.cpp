#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include "scripting/bindings/BindMapValidation.hpp"
#include "map/TiledMap.hpp"

#include <SDL3/SDL.h>
#include <optional>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

struct ValidationResult {
    bool                     ok{true};
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

static ValidationResult ValidateMap(const TiledMap &map) {
    ValidationResult r;

    if (map.tileW <= 0 || map.tileH <= 0) {
        r.errors.push_back("map tile size is zero or negative (tileW="
            + std::to_string(map.tileW) + " tileH=" + std::to_string(map.tileH) + ")");
        r.ok = false;
    }

    if (map.tilesets.empty()) {
        r.warnings.push_back("map has no tilesets — tile layers will render nothing");
    }

    int maxGid = 0;
    for (const auto &ts : map.tilesets) {
        if (ts.tileW <= 0 || ts.tileH <= 0 || ts.columns <= 0) {
            r.errors.push_back("tileset firstGid=" + std::to_string(ts.firstGid)
                + " has invalid tile dimensions or columns");
            r.ok = false;
            continue;
        }
        if (ts.imagePath.empty()) {
            r.warnings.push_back("tileset firstGid=" + std::to_string(ts.firstGid)
                + " has no imagePath — will not render");
        }
        maxGid = std::max(maxGid, ts.firstGid);
    }
    (void)maxGid;

    for (std::size_t i = 0; i < map.tileLayers.size(); ++i) {
        const auto &tl = map.tileLayers[i];
        const std::string id = "tilelayer[" + std::to_string(i) + "]"
                             + (tl.name.empty() ? "" : (" '" + tl.name + "'"));

        if (tl.width <= 0 || tl.height <= 0) {
            r.errors.push_back(id + ": width/height is zero or negative");
            r.ok = false;
            continue;
        }

        const int expected = tl.width * tl.height;
        if (static_cast<int>(tl.data.size()) != expected) {
            r.errors.push_back(id + ": data length " + std::to_string(tl.data.size())
                + " != width*height " + std::to_string(expected));
            r.ok = false;
        }

        int nonZero = 0;
        for (int gid : tl.data) {
            if (gid < 0) {
                r.errors.push_back(id + ": contains negative GID " + std::to_string(gid));
                r.ok = false;
                break;
            }
            if (gid > 0) ++nonZero;
        }
        if (nonZero == 0) {
            r.warnings.push_back(id + ": all tiles are empty (GID 0) — nothing will render");
        }
    }

    const float mapPixW = (map.tileLayers.empty() ? 0.f :
        static_cast<float>(map.tileLayers[0].width  * map.tileW));
    const float mapPixH = (map.tileLayers.empty() ? 0.f :
        static_cast<float>(map.tileLayers[0].height * map.tileH));

    for (std::size_t li = 0; li < map.objectLayers.size(); ++li) {
        const auto &ol = map.objectLayers[li];
        const std::string lid = "objectlayer[" + std::to_string(li) + "]"
                              + (ol.name.empty() ? "" : (" '" + ol.name + "'"));

        for (const auto &mo : ol.objects) {
            const std::string oid = lid + " object id=" + std::to_string(mo.id)
                + (mo.name.empty() ? "" : " '" + mo.name + "'");

            if (mo.type.empty()) {
                r.warnings.push_back(oid + ": has no type/class — scripts may ignore it");
            }
            if (mo.x < 0.f || mo.y < 0.f) {
                r.warnings.push_back(oid + ": position (" + std::to_string(mo.x)
                    + "," + std::to_string(mo.y) + ") is off map origin");
            }
            if (mapPixW > 0.f && mapPixH > 0.f) {
                if (mo.x > mapPixW || mo.y > mapPixH) {
                    r.warnings.push_back(oid + ": position (" + std::to_string(mo.x)
                        + "," + std::to_string(mo.y) + ") is outside map bounds ("
                        + std::to_string(mapPixW) + "x" + std::to_string(mapPixH) + ")");
                }
            }
            if (mo.w < 0.f || mo.h < 0.f) {
                r.errors.push_back(oid + ": negative width or height");
                r.ok = false;
            }
        }
    }

    return r;
}

// ---------------------------------------------------------------------------
// Binding
// ---------------------------------------------------------------------------

void BindMapValidation(sol::state &lua, std::optional<TiledMap> *lastMap) {
    auto w = lua["world"].get<sol::table>();

    // Capture pointer-to-optional (not pointer-to-value) so this lambda
    // always sees whatever load_tiled_map wrote, even if it ran after
    // BindMapValidation was called.
    w.set_function("validate_map",
        [&lua, lastMap]() -> sol::table {
            auto tbl = lua.create_table();

            if (!lastMap || !lastMap->has_value()) {
                tbl["ok"] = false;
                auto errs = lua.create_table();
                errs[1] = "no map has been loaded yet";
                tbl["errors"]   = errs;
                tbl["warnings"] = lua.create_table();
                SDL_Log("[map validate] ERROR no map has been loaded yet");
                return tbl;
            }

            const ValidationResult r = ValidateMap(lastMap->value());

            tbl["ok"] = r.ok;

            auto warns = lua.create_table();
            for (std::size_t i = 0; i < r.warnings.size(); ++i)
                warns[static_cast<int>(i + 1)] = r.warnings[i];
            tbl["warnings"] = warns;

            auto errs = lua.create_table();
            for (std::size_t i = 0; i < r.errors.size(); ++i)
                errs[static_cast<int>(i + 1)] = r.errors[i];
            tbl["errors"] = errs;

            for (const auto &w_ : r.warnings)
                SDL_Log("[map validate] WARN  %s", w_.c_str());
            for (const auto &e_ : r.errors)
                SDL_Log("[map validate] ERROR %s", e_.c_str());

            return tbl;
        });
}
