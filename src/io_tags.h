#pragma once
#ifndef CATA_SRC_IO_TAGS_H
#define CATA_SRC_IO_TAGS_H

namespace io
{

class JsonArrayInputArchive;
class JsonArrayOutputArchive;
class JsonObjectInputArchive;
class JsonObjectOutputArchive;




struct array_archive_tag {
    using InputArchive = JsonArrayInputArchive;
    using OutputArchive = JsonArrayOutputArchive;
};




struct object_archive_tag {
    using InputArchive = JsonObjectInputArchive;
    using OutputArchive = JsonObjectOutputArchive;
};

}

#endif
