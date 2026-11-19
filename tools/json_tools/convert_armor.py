#!/usr/bin/env python3





import argparse
import json
import os


args = argparse.ArgumentParser()
args.add_argument("dir", action="store", help="specify json directory")
args_dict = vars(args.parse_args())



all_filepaths = set()

items_at_path = dict()

all_armor_defs = dict()

transferred_keys = [
    "material",
    "material_thickness",
    "environmental_protection",
    "environmental_protection_with_filter"
]

accepted_types = [
    "AMMO",
    "GUN",
    "ARMOR",
    "PET_ARMOR",
    "TOOL",
    "TOOLMOD",
    "TOOL_ARMOR",
    "BOOK",
    "COMESTIBLE",
    "EGINE",
    "WHEEL",
    "GUNMOD",
    "MAGAZINE",
    "BATTERY",
    "GENERIC",
    "BIONIC_ITEM"
]


def id_or_abstract(jo):
    if "id" in jo:
        return jo["id"]
    return jo["abstract"]




def extract_armor_add_path(path):
    with open(path, "r", encoding="utf-8") as json_file:
        try:
            json_data = json.load(json_file)
        except json.JSONDecodeError:
            print(f"Json Decode Error at {path}")
            print("Ensure that the file is a JSON file consisting of"
                  " an array of objects!\n")
            return None

        for jo in json_data:
            if type(jo) is not dict:
                print(f"Incorrectly formatted JSON data at {path}")
                print("Ensure that the file is a JSON file consisting"
                      " of an array of objects!\n")
                break

            all_filepaths.add(path)

            if ("type" not in jo or
               jo["type"] not in accepted_types):
                continue

            def_id = id_or_abstract(jo)

            if def_id not in items_at_path:
                items_at_path[def_id] = set()
            items_at_path[def_id].add(path)


            '''
            color_discard = "\x1b[31m"
            color_used = "\x1b[32m"
            color_end = "\x1b[0m"
            '''

            if (def_id in all_armor_defs and not
               ("copy-from" in all_armor_defs[def_id] and
                   all_armor_defs[def_id]["copy-from"] == def_id)):
                print(f"WARNING: {def_id} defined twice, at {path},"
                      " second definition discarded")

                '''
                print("Defined at:")
                for path in items_at_path[def_id]:
                    print(f"! '{def_id}': {color_discard}{path}{color_end}")
                '''
                print("This can occur due to mods overriding items, "
                      "ensure that the assigned values are accurate!\n")
                return
            elif def_id in all_armor_defs:
                print(f"WARNING: {def_id} defined twice, "
                      "using definition from {path}")

                '''
                print("Defined at:")
                for path in items_at_path[def_id]:
                    print(f"! '{def_id}': {color_used}{path}{color_end}")
                '''
                print("This can occur due to mods overriding items, "
                      "ensure that the assigned values are accurate!\n")

            all_armor_defs[def_id] = jo


def assign_if_absent(dat, key, value):
    if key not in dat:
        dat[key] = value



def read_armor_data(jo, dat):
    for key in transferred_keys:
        if key in jo:
            assign_if_absent(dat, key, jo[key])




def get_armor_data_rec(jo, dat):

    if all(key in dat for key in transferred_keys):
        return

    read_armor_data(jo, dat)

    if "copy-from" in jo:
        get_armor_data_rec(all_armor_defs[jo["copy-from"]], dat)





def get_armor_data(jo):
    dat = dict()


    read_armor_data(jo, dat)
    if ("armor" not in jo and
       (len(dat) == 0 or (len(dat) == 1 and "material" in dat))):
        return dict()

    get_armor_data_rec(jo, dat)
    return dat




def actually_distinct(jo):
    if "copy-from" not in jo:
        return True

    dat = dict()

    read_armor_data(jo, dat)

    copied = get_armor_data(all_armor_defs[jo["copy-from"]])

    for key in dat:
        if key not in copied:
            return True
        if copied[key] != dat[key]:
            return True

    return False



def merge_with_armor(jo, dat):
    orig = all_armor_defs[jo["copy-from"]]
    while "armor" not in orig:
        if "copy-from" not in orig:
            return [dat]
        orig = all_armor_defs[orig["copy-from"]]

    out = orig["armor"]
    for portion in out:
        for key in dat:
            portion[key] = dat[key]

    return out


def handle_armor_data(jo):
    armor = jo["armor_data"]
    dat = dict()
    for key in transferred_keys:
        if key not in armor:
            continue
        dat[key] = armor[key]
        del armor[key]

    curjo = jo
    while "material" not in curjo:
        out = id_or_abstract(curjo)
        if "copy-from" not in curjo:
            print(f"{out}: malformed armor def - materials are required"
                  " for armor items")
            return
        if curjo["copy-from"] not in all_armor_defs:
            print(f"{out}: does not copy-from an armor item!"
                  "material will need manual addition")
            return
        if curjo["copy-from"] == out:
            print("AAAAAAA")
            print(f"{out}")
        curjo = all_armor_defs[curjo["copy-from"]]

    dat["material"] = curjo["material"]

    if "armor" in armor:
        for portion in armor["armor"]:
            portion.update(dat)

    return jo


def gen_new(path):
    change = False
    with open(path, "r", encoding="utf-8") as json_file:
        try:
            json_data = json.load(json_file)
        except json.JSONDecodeError:
            print(f"Json Decode Error at {path}")
            print("Ensure that the file is a JSON file consisting of"
                  " an array of objects!")
            return None

        for jo in json_data:
            if ("type" not in jo or
               jo["type"] != "ARMOR" and
               jo["type"] != "TOOL_ARMOR" and
               "armor_data" not in jo):
                continue

            if "armor_data" in jo:
                change = True
                handle_armor_data(jo)
                continue

            dat = get_armor_data(jo)
            if len(dat) == 0:
                continue

            change = True

            if "armor" not in jo:
                if actually_distinct(jo):
                    if "copy-from" in jo:
                        jo["armor"] = merge_with_armor(jo, dat)
                    else:
                        jo["armor"] = [dat]
            else:
                for portion in jo["armor"]:
                    portion.update(dat)

            for key in transferred_keys:
                if key != "material" and key in jo:
                    del jo[key]

        return json_data if change else None


print("extracting data...")
for root, directories, filenames in os.walk(args_dict["dir"]):
    for filename in filenames:
        if not filename.endswith(".json"):
            continue

        path = os.path.join(root, filename)
        extract_armor_add_path(path)

print("Done!")
print("Modifying...")
for path in all_filepaths:
    new = gen_new(path)
    if new is not None:
        with open(path, "w", encoding="utf-8") as jf:
            json.dump(new, jf, ensure_ascii=False)
        os.system(f"./tools/format/json_formatter.cgi {path}")
