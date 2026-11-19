#pragma once
#ifndef CATA_SRC_LOADING_UI_H
#define CATA_SRC_LOADING_UI_H

#include <string>

namespace loading_ui
{
void show( const std::string &context, const std::string &step );
void done();



void release_gpu_resources();
}

#endif
