# procyon
This is my current ongoing project in which I am trying to create a Co-Op Action-Adventure game for PC and PlayStation Portable.
Why the PSP? Well, it was my first childhood console and also the reason I have started learning low-level programming in the first place.
I am very curious to see what 333MHz MIPS R4000 CPU, 32MB of RAM and 2MB of GPU memory is capable of.

## 11/05/2023
I've decided not to use Model 3D (M3D) format in the client app directly. To render a model on PC or PSP I need to convert the data from
M3D's data representation to something the engine could use. I wasn't concerned too much about desktop platforms - there is plenty of CPU
power to do necessary conversions, but on the PSP I don't have CPU power or large amounts of memory to deal with. I think what I am going
to do about it is to create two custom model formats, one for PC and one for PSP. Each of the platforms would get a format that is easy for
them to load, ideally with minimal CPU and memory overhead. I am not experienced enough to write an entire Blender Exporter from scratch,
but since I've already learned how to use M3D format, I decided to create a conversion tool.

## 07/05/2023 Models
Once I've got basic network connection going on I thought it is time to work on rendering 3D models. I decided to use Model 3D SDK and
file format, it promised small files and straightforward way to load models. With Model 3D I was able to load and display a textured model on PC.
![image](https://github.com/Sokus/procyon/assets/26815390/c1a24a6d-d0a3-4364-b84a-dd0b3177396f)

## 22/04/2023 First network connection
My biggest achievement with the project so far was getting PC and PSP client to connect to the same server. Networking abstraction, communication
protocol and bit-serialization I have used in my [Engineering Thesis](https://github.com/Sokus/engineering-thesis) were completely rewritten - two times actually - which reduced the amount of
code and helped detect some bugs introduced when implementing everything for the first time.

https://github.com/Sokus/procyon/assets/26815390/37ecf4bd-5a91-48fb-a89b-79d565594c80

