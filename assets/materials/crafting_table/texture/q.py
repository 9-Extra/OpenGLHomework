import cv2
import numpy
import os

# def trans(path: str):
#     # m e r -> e r 
#     # r g b -> r b 0
#     # b g r -> 0 b r
#     image = cv2.imread(path)
#     image[:, : , 1] = image[:, :, 0]
#     image[:, :, 0] = 0
#     cv2.imwrite(path, image)

# base_dir = R"D:\programming\OpenGL开发包\assets\materials\crafting_table\texture"
# for p in os.listdir(base_dir):
#     if (p.endswith("mer.png")):
#         print(os.path.join(base_dir, p))
#         trans(os.path.join(base_dir, p))

def flip(path: str):
    image = cv2.imread(path)
    image = cv2.flip(image, 0)
    cv2.imwrite(path, image)

base_dir = R"D:\programming\OpenGL开发包\assets\materials\crafting_table\texture"
for p in os.listdir(base_dir):
    if (p.endswith(".png")):
        print(os.path.join(base_dir, p))
        flip(os.path.join(base_dir, p))
 

