#include "Material.h"
#include <iostream>

FIBITMAP *freeimage_load_and_convert_image(const std::string &image_path) {
    FIBITMAP *pImage_ori = FreeImage_Load(FreeImage_GetFileType(image_path.c_str(), 0), image_path.c_str());
    if (pImage_ori == nullptr) {
        std::cerr << "Failed to load image: " << image_path << std::endl;
        exit(-1);
    }
    FIBITMAP *pImage = FreeImage_ConvertTo24Bits(pImage_ori);
    // FreeImage_FlipVertical(pImage); // 翻转，适应opengl的方向
    FreeImage_Unload(pImage_ori);

    return pImage;
}
