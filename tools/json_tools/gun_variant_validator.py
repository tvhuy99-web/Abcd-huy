#!/usr/bin/env python3

import argparse
import json
import os

args = argparse.ArgumentParser()
args.add_argument("dir", action="store", help="specify json directory")
args.add_argument("-v", "--verbose", action="count", default=0,
                  help="extra debugging output, '-v' or '-vv'")
args.add_argument("-c", "--combine", action="store_true",
                  help="check for guns that should be combined as variants")
args.add_argument("-i", "--identifier", action="store_true",
                  help="check for guns that do not share an identifier "
                       "with their magazines")
args.add_argument("--only-good-idents", action="store_true",
                  help="if checking identifiers, output valid identifiers")
args.add_argument("-n", "--name", action="store_true",
                  help="check for guns with non-descriptive names")
args.add_argument("--no-failure", action="store_true",
                  help="do not exit as a failure if any of the checks fail")
args.add_argument("-t", "--table", action="store_true",
                  help="output collected data as a table and exit")
args_dict = vars(args.parse_args())

all_jos = dict()
VERBOSE = args_dict["verbose"]
LOADED_TYPES = {"GUN", "MAGAZINE"}
INHERITED_KEYS = [
    "abstract",
    "id",
    "type",
    "subtypes",
    "name",
    "ammo",
    "flags",
    "volume",
    "weight",
    "modes",
    "barrel_length",
    "ranged_damage",
    "reload",
    "longest_side",
    "linkage",
    "pocket_data",
    "dispersion",
    "recoil",
    "barrel_volume",
    "blackpowder_tolerance",
]
WEIGHT_UNITS = {"kg": 1000, "g": 1}
VOLUME_UNITS = {"L": 1000, "ml": 1}
LENGTH_UNITS = {"mm": 1}
UNIT_KEYS = [
    "weight",
    "volume",
    "barrel_volume",
    "longest_side",
    "barrel_length",
]
UNIT_UNITS = {
    "weight": WEIGHT_UNITS,
    "volume": VOLUME_UNITS,
    "barrel_volume": VOLUME_UNITS,
    "longest_side": LENGTH_UNITS,
    "barrel_length": LENGTH_UNITS,
}
GUNS_BLACKLIST = {

    "brogyeki",
    "khuunaofaai",
    "exodii_plasma_fan",
    "exodii_plasma_projectile",
    "nl_chem_thrower",
}
AMMO_BLACKLIST = {
    "arrow",
    "bolt",
    "payload",
    "chemical_spray",
    "fishspear",
    "nail",
    "paintball",
    "signal_flare",
    "water",
}
VARIANT_CHECK_BLACKLIST = {
    "pamd68rubik",

}
VARIANT_CHECK_PAIR_BLACKLIST = {

    ("pamd68", "pamd68mountable"),
    ("type99", "type99_sniper"),
    ("ithaca37", "remington_1100"),
    ("browning_a5", "benelli_sa"),
    ("m1014", "remington_870_express"),




}
IDENTIFIER_CHECK_BLACKLIST = {

    "ar12_shotgun",
    "bigun",
    "fn_fal_semi",
    "gfy_1",
    "hpt3895",
    "m1a",
    "m2carbine",
    "mdrx",
    "rfb_308",
    "srm_1216",
}
NAME_CHECK_BLACKLIST = {

    "AT4",
    "atgm_launcher",
    "bbgun",
    "brogyaga",
    "fn_p90",
    "hi_power_40",
    "hptjcp",
    "iwi_tavor_x95_300blk",
    "m1911a1_38super",
    "m2010",
    "m203",
    "m320_mod",
    "mgl",
    "obrez",
    "p226_357sig",
    "p320_357sig",
    "pressin",
    "pseudo_m203",
    "px4_40",
    "raging_bull",
    "scar_l",
    "shotgun_410",
    "sig_40",
    "sig_mcx_rattler_sbr",
    "weatherby_5",
    "win70",
    "xedra_gun",
}

BAD_IDENTIFIERS = [
    "10mm",
    ".22",
    ".30-06",
    ".300",
    ".338",
    ".357",
    ".380",
    ".38 Super",
    ".38",
    ".32",
    ".40",
    ".44",
    ".450",
    ".45",
    ".500",
    ".50",
    "5x50mm",
    "5.7mm",
    "7.7mm",
    "8x40mm",
    "9x19mm",
    "magazine-fed",
    "-round",
    "clip",
    "magazine",
    "stripper",
    "speedloader",
]

BAD_COMMON_TOKENS = {
    "rifle",
    "carbine",
    "pistol",
    "revolver",
    "shotgun",
    "handgun",
    "five-shooter",
    "six-shooter",
    "SMG",
    "submachine gun",
    "grenade launcher",
    "ing"
}

SHORT_COMMON_TOKENS = {
    "AI",
    "AK",
    "FN",
    "G3",
    "M9",
}
TYPE_DESCRIPTORS = [
    "automagnum",
    "blunderbuss",
    "cannon",
    "carbine",
    "coilgun",

    "combination gun",
    "derringer",
    "flamethrower",
    "flintlock",

    "FSP",
    "gatling gun",
    "grenade launcher",
    "hand cannon",
    "handgun",
    "HMG",

    "HWP",
    "launcher",
    "lever gun",
    "LMG",
    "machine gun",
    "minigun",

    "operational briefcase",
    "pistol",
    "railgun",
    "revolver",
    "rifle",
    "shotgun",
    "five-shooter",
    "six-shooter",
    "SMG",
    "submachine gun",
    "trenchgun",
]
DUPE_CHECK_BLACKLIST = [

]

"""
JSON parsing functions
"""


def parse_unit(val, stops):

    if type(val) is not str:
        return val
    total = 0
    unit = 0
    sign = 1
    unit_start = -1
    eaten = 0
    while len(val[eaten:]) != 0:
        if val[eaten].isdigit():
            unit *= 10
            unit += ord(val[eaten]) - ord('0')
        elif val[eaten] == '-':
            sign = -1

        elif not val[eaten].isspace():
            unit_start = eaten if unit_start == -1 else unit_start

            if val[unit_start:eaten + 1] in stops:
                unit *= stops[val[unit_start:eaten + 1]]
                total += unit * sign
                unit = 0
                sign = 1
                unit_start = -1
        eaten += 1
    return total


def simplify_gun_data(jo):

    if "modes" in jo:
        modes = []
        for mode in jo["modes"]:
            modes.append(mode[2])
        jo["modes"] = sorted(modes)
    else:
        jo["modes"] = [1]

    if "ranged_damage" in jo:
        if type(jo["ranged_damage"]) is not dict:
            raise ValueError("Multiple damage types not supported")
        jo["ranged_damage"] = jo["ranged_damage"]["amount"]
    else:
        jo["ranged_damage"] = 0

    if "pocket_data" not in jo:
        if VERBOSE:
            print(f" - Removing {jo['id']}, conversion kit gun")
        return False
    for data in jo["pocket_data"]:

        if data["pocket_type"] == "MAGAZINE":

            quantity = next(iter(data["ammo_restriction"].values()))

            if (len(data["ammo_restriction"]) != 1 and
               len(set(data["ammo_restriction"].values())) != 1):
                raise ValueError("Only one quantity allowed for ammo")

            jo["magazines"] = [quantity]

            if "allowed_speedloaders" in data:
                jo["speedloaders"] = data["allowed_speedloaders"]

        elif data["pocket_type"] == "MAGAZINE_WELL":
            jo["magazines"] = sorted(data["item_restriction"])
        else:
            raise ValueError("Guns with other pockets not supported")

    del jo["pocket_data"]
    return True


def simplify_object(jo):

    if "id" not in jo or jo["id"] in GUNS_BLACKLIST:
        if VERBOSE:
            name = str(jo)
            if "id" in jo:
                name = jo["id"]
            if "abstract" in jo:
                name = jo["abstract"]
            print(f" - Removing {name}, blacklisted or abstract")
        return False

    req_keys = {"weight", "volume", "ammo", "id"}
    all_keys = req_keys | set(INHERITED_KEYS)

    removed = list(filter(lambda key: key not in all_keys, jo.keys()))

    for key in removed:
        del jo[key]

    for key in filter(lambda key: key not in jo, req_keys):
        if VERBOSE:
            print(f" - Removing {jo['id']}, lacking {key}")
        return False

    if "flags" in jo and "PSEUDO" in jo["flags"]:
        if VERBOSE:
            print(f" - Removing  {jo['id']}, pseudo gun")
        return False

    return simplify_gun_data(jo)


def name_of(jo):
    if type(jo["name"]) is dict:
        if "str" in jo["name"]:
            return jo["name"]["str"]
        else:
            return jo["name"]["str_sp"]
    return jo["name"]


"""
JSON loading functions
"""


def extract_jos(path):
    with open(path, "r", encoding="utf-8") as json_file:
        try:
            json_data = json.load(json_file)
        except json.JSONDecodeError:
            print(f"Json Decode Error at {path}")
            print("Ensure that the file is a JSON file consisting of"
                  " an array of objects!")
            return None

        for jo in json_data:
            if type(jo) is not dict:
                print(f"Incorrectly formatted JSON data at {path}")
                print("Ensure that the file is a JSON file consisting"
                      " of an array of objects!")
                break

            if (jo["type"] != "ITEM") or ("subtypes" not in jo) or \
               LOADED_TYPES.isdisjoint(jo["subtypes"]):
                continue

            ident = jo["id"] if "id" in jo else jo["abstract"]

            all_jos[ident] = jo


def copy_from_extend(jo, data):
    if "extend" not in jo:
        return
    subobject = jo["extend"]
    for key in INHERITED_KEYS:
        if key not in subobject:
            continue
        for val in subobject[key]:
            if key not in data:
                data[key] = [val]
            else:
                data[key].append(val)


def copy_from_delete(jo, data):
    if "delete" not in jo:
        return
    subobject = jo["delete"]
    for key in INHERITED_KEYS:
        if key not in subobject:
            continue
        for val in subobject[key]:
            if key not in data:
                raise ValueError(f"Invalid delete on {key}")
            else:
                data[key].remove(val)


def copy_from_relative(jo, data):
    if "relative" not in jo:
        return
    subobject = jo["relative"]
    for key in INHERITED_KEYS:
        if key not in subobject:
            continue
        if key not in data:
            raise ValueError(f"Invalid relative on {key}")
        if key in UNIT_KEYS:
            data[key] += parse_unit(subobject[key], UNIT_UNITS[key])
        elif (key == "ranged_damage" and
              type(data[key]) is dict and
              type(subobject[key]) is dict):
            data[key]["amount"] += subobject[key]["amount"]
        elif key in {"dispersion"}:
            data[key] += subobject[key]
        else:
            raise ValueError(f"Relative for {key} not supported")


def copy_from_proportional(jo, data):
    if "proportional" not in jo:
        return
    subobject = jo["proportional"]
    for key in INHERITED_KEYS:
        if key not in subobject:
            continue
        if key not in data:
            raise ValueError(f"Invalid proportional on {key}")
        if (key == "ranged_damage" and
           type(data[key]) is dict and
           type(subobject[key]) is dict):
            data[key]["amount"] *= subobject[key]["amount"]
        elif key in UNIT_KEYS or key in {"reload", "dispersion"}:
            data[key] *= subobject[key]
        else:
            raise ValueError(f"Proportional for {key} not supported")


def do_copy_from(jo):
    data = {}
    if "copy-from" in jo and jo["copy-from"] in all_jos:
        data = do_copy_from(all_jos[jo["copy-from"]])

    for key in INHERITED_KEYS:
        if key in jo:
            if key in UNIT_KEYS:
                data[key] = parse_unit(jo[key], UNIT_UNITS[key])
            elif key == "name":
                data[key] = name_of(jo)
            else:
                data[key] = jo[key]

    copy_from_extend(jo, data)
    copy_from_delete(jo, data)
    copy_from_relative(jo, data)
    copy_from_proportional(jo, data)

    return data


def load_all_json(directory):
    for root, directories, filenames in os.walk(directory):
        for filename in filenames:
            if not filename.endswith(".json"):
                continue
            if "obsolet" in root + filename:

                continue
            path = os.path.join(root, filename)
            extract_jos(path)

    for gun in all_jos.keys():
        new = do_copy_from(all_jos[gun])
        all_jos[gun] = new


"""
CHECK 1: Should a gun be a variant?
"""


def guns_are_similar(jo, gun):

    def in_range(lhs, rhs, bucket_size):
        return abs((lhs // bucket_size) - (rhs // bucket_size)) < 2

    def in_pct(lhs, rhs, pct):
        low = 1 - pct
        high = 1 + pct
        return ((lhs >= rhs * low and lhs <= rhs * high) or
                (rhs >= lhs * low and rhs <= lhs * high))

    def sets_match(lhs, rhs):
        return len(set(lhs) ^ set(rhs)) == 0

    if (gun["id"] in VARIANT_CHECK_BLACKLIST or
       jo["id"] in VARIANT_CHECK_BLACKLIST):
        return False
    if not sets_match(jo["ammo"], gun["ammo"]):
        return False
    if not sets_match(jo["modes"], gun["modes"]):
        return False
    if not in_range(jo["volume"], gun["volume"], 250):
        return False
    if not in_range(jo["weight"], gun["weight"], 250):
        return False
    if not in_range(jo["ranged_damage"], gun["ranged_damage"], 2):
        return False

    if ("longest_side" in jo and "longest_side" in gun and
       not in_range(jo["longest_side"], gun["longest_side"], 50)):
        return False

    if not in_pct(jo["dispersion"] if "dispersion" in jo else 0,
                  gun["dispersion"] if "dispersion" in gun else 0, 0.05):
        return False

    if ("speedloaders" in jo) != ("speedloaders" in gun):
        return False

    if ("speedloaders" in jo and "speedloaders" in gun and
       not sets_match(jo["speedloaders"], gun["speedloaders"])):
        return False
    if not sets_match(jo["magazines"], gun["magazines"]):
        return False
    if not in_range(jo["reload"] if "reload" in jo else 0,
                    gun["reload"] if "reload" in gun else 0,
                    125):
        return False
    if ("barrel_length" in jo and "barrel_length" in gun and







       not in_range(jo["barrel_length"], gun["barrel_length"], 10)):
        return False

    return True


def find_variants(all_guns):
    similar_guns = []
    for i in range(len(all_guns)):
        gun = all_guns[i]

        for compared in all_guns[i + 1:]:
            lid = gun["id"]
            rid = compared["id"]

            if (guns_are_similar(gun, compared) and
                not ((lid, rid) in VARIANT_CHECK_PAIR_BLACKLIST or
                     (rid, lid) in VARIANT_CHECK_PAIR_BLACKLIST)):
                similar_guns.append((gun, compared))
    return similar_guns


"""
CHECK 2: Are the default gun names linking the gun and it's mags
"""


def longest_common_substring(dat):
    substr = ''
    if len(dat) > 1 and len(dat[0]) > 0:
        for i in range(len(dat[0])):
            for j in range(len(dat[0]) - i + 1):
                if j > len(substr) and all(dat[0][i:i + j] in x for x in dat):
                    substr = dat[0][i:i + j]
    return substr


def clean_names(names):
    ret = []
    for name in names:
        for token in BAD_IDENTIFIERS:
            name = name.replace(token, "")
        ret.append(name)
    return ret


def common_token(names):
    names = clean_names(names)

    common_token = " ".join(longest_common_substring(names).split())

    if len(common_token) < 3 and common_token not in SHORT_COMMON_TOKENS:
        return None

    if common_token in BAD_COMMON_TOKENS:
        return None
    if VERBOSE > 1:
        print(f" --- Token '{common_token}': {names}")
    return common_token


def print_identifier_info(token, gun, magazines):
    mags = ""
    alt_mags = ""
    for mag in magazines:
        name = name_of(all_jos[mag]) if mag in all_jos else ""

        if "STANAG" in name:
            alt_mags += mag + ", "
            continue
        mags += mag + f" ({name})" + ", "
    print(token)
    print("  " + gun["id"] + " (" + name_of(gun) + ")")
    if (len(mags) > 0):
        print("  " + mags)
    if (len(alt_mags) > 0):
        print("  " + alt_mags)


def find_identifiers(all_guns):

    good_tokens = []

    bad_tokens = []
    for gun in all_guns:
        if gun["id"] in IDENTIFIER_CHECK_BLACKLIST:
            continue

        names = [name_of(gun)]

        mags = []
        for mag in gun["magazines"]:
            if mag not in all_jos:
                if VERBOSE and not type(mag) is not str:
                    print(f" - Not checking magazine {mag} for {gun['id']}")
                continue
            mags.append(mag)

        for mag in mags:
            name = name_of(all_jos[mag])

            if "STANAG" not in name and "linkage" not in all_jos[mag]:
                names.append(name)

        if len(names) < 2:
            continue

        token = common_token(names)
        if token is None:
            bad_tokens.append((gun, mags))
        else:
            good_tokens.append((gun, mags, token))
    return good_tokens, bad_tokens


"""
CHECK 3: names say what they are, and aren't just gobbledygook
"""


def find_bad_names(all_guns):
    bad_names = []
    for gun in all_guns:
        if gun["id"] in NAME_CHECK_BLACKLIST:
            continue
        name = name_of(gun)
        good = False
        for type_descriptor in TYPE_DESCRIPTORS:
            if type_descriptor.upper() in name.upper():
                good = True
                break
        if not good:
            bad_names.append((gun["id"], name))
    return bad_names


"""
CHECK 4: No dupes!
"""


def remove_blacklisted_dupes(guns):
    if len(guns) == 1:
        return guns
    guns_set = set(map(lambda gun: gun["id"], guns))
    for blacklist_set in DUPE_CHECK_BLACKLIST:
        all_match = True
        for gun in blacklist_set:
            if gun not in guns_set:
                print(gun, "not in", guns_set)
                all_match = False
                break
        if all_match:
            guns = list(filter(lambda g: g["id"] not in blacklist_set, guns))
    return guns


def find_dupe_names(all_guns):
    error = False
    all_names = {}
    for gun in all_guns:
        name = name_of(gun) + str(sorted(gun["ammo"]))

        for mag_type in gun.get("magazines"):
            if type(mag_type) is int:
                name += f",{mag_type}-round"

        if name in all_names:
            all_names[name].append(gun)
        else:
            all_names[name] = [gun]
    for key, value in all_names.items():
        value = remove_blacklisted_dupes(value)
        if len(value) < 2:
            continue
        error = True
        out = f"ERROR: Guns have the same name, ammo, capacity ({key}):\n  "
        for gun in value:
            out += f" '{gun['id']}',"
        print(out)
    return error


"""
Driver functions
"""


def check_combination(all_guns):
    if not args_dict["combine"]:
        return False

    print("==== VARIANT COMBINATION ====")
    similar_guns = find_variants(all_guns)
    for pair in similar_guns:
        print(f"ERROR: Guns '{pair[0]['id']}' and '{pair[1]['id']}' are"
              " too similar and should be combined")

    return len(similar_guns) > 0


def check_identifiers(all_guns):
    if not args_dict["identifier"]:
        return False

    print("====  IDENTIFIER  CHECK  ====")
    good_tokens, bad_tokens = find_identifiers(all_guns)
    if args_dict["only_good_idents"]:
        for good in good_tokens:
            print_identifier_info(good[2], good[0], good[1])
    else:
        for bad in bad_tokens:
            error_str = "This gun and it's magazines lack a shared identifier:"
            print_identifier_info(f"ERROR: {error_str}", bad[0], bad[1])
        if VERBOSE:
            good_token_list = {}
            for good in good_tokens:
                if good[2] in good_token_list:
                    good_token_list[good[2]].append(good[0]["id"])
                else:
                    good_token_list[good[2]] = [good[0]["id"]]

            print("The following valid identifiers were found.",
                  "Please check to ensure they make sense.")

            for token in sorted(good_token_list):
                print(f" - '{token}': {good_token_list[token]}")

    return len(bad_tokens) > 0


def check_names(all_guns):
    if not args_dict["name"]:
        return False
    print("====  DESCRIPTIVE NAMES  ====")
    bad_names = find_bad_names(all_guns)
    for bad in bad_names:
        print(f"ERROR: Gun '{bad[1]}' ({bad[0]}) lacks a descriptive name")

    dupes = find_dupe_names(all_guns)

    return len(bad_names) > 0 or dupes


csv = True


def insert_start_char():
    return "" if csv else "| "


def insert_separator():
    return "," if csv else " | "


def table(all_guns):

    skipped = {
        "type",
        "subtypes",
        "barrel_volume",
        "blackpowder_tolerance",
        "pocket_data",
        "recoil"
    }

    defaults = {
        "flags": [],
        "reload": 0,
        "speedloaders": [],
    }

    keys = INHERITED_KEYS + ["magazines", "speedloaders"]

    out = insert_start_char()
    for key in keys:
        if key in skipped:
            continue
        out += key
        out += insert_separator()
    print(out)

    if not csv:
        out = insert_start_char()
        for key in keys:
            if key in skipped:
                continue
            out += "----"
            out += insert_separator()
        print(out)

    for gun in all_guns:
        out = insert_start_char()
        for key in keys:
            if key in skipped:
                continue
            if key in gun and type(gun[key]) is list:
                for val in gun[key]:
                    out += "'" + str(val) + "' "
            else:
                out += str(gun[key] if key in gun else defaults.get(key, "?"))
            out += insert_separator()
        print(out)


def main():

    load_all_json(args_dict["dir"])

    all_guns = []
    for jo in all_jos.values():
        if "GUN" not in jo["subtypes"]:
            continue
        if ("flags" in jo and
            ("PRIMITIVE_RANGED_WEAPON" in jo["flags"] or
             "PSEUDO" in jo["flags"])):
            continue
        if not simplify_object(jo):
            continue

        if len(set(jo["ammo"]).intersection(AMMO_BLACKLIST)) != 0:
            continue
        all_guns.append(jo)
    if args_dict["table"]:
        table(all_guns)
        os.sys.exit(0)

    failed = False
    if check_combination(all_guns):
        failed = True
    if check_identifiers(all_guns):
        failed = True
    if check_names(all_guns):
        failed = True

    if failed and not args_dict["no_failure"]:
        os.sys.exit(1)
    else:
        os.sys.exit(0)


if __name__ == "__main__":
    main()
