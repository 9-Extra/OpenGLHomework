#include "simple_json.h"
#include <iostream>

int main(){
    std::unique_ptr<SimpleJson::JsonObject> json = SimpleJson::parse_file(R"(D:\programming\OpenGL开发包\workspace\homework3-余天一2020270901005\assets\materials\wood_floor_deck\wood_floor_deck_1k.gltf)");
    std::cout << *json;
    return 0;
}