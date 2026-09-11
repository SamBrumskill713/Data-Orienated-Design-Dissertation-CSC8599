# Data-Oriented Design Dissertation
For my dissertation, I decided to do a performance comparison between Data-Oriented Design and Object Oriented Design. 
I did this by converting the framework made in the game technologies coursework from Object-Oriented to Data-Oriented. 
The main reason for this was to see how game performance could be affected by a change in paradigm that focuses on efficient use of cache hierarchy and cache lines. The results showed that Data-Oriented Design was much more performant when compared to Object-Oriented Programming.

![image_alt](https://github.com/SamBrumskill713/Data-Orienated-Design-Dissertation-CSC8599/blob/6c8d09a8a783ce82dde331cbf3c43c412c860a03/DOD-dissertation-screenshot.png)

## Main Concepts Covered
- Data-Oriented Design
- Object-Oriented Programming
- C++
- CPU Architecture
- Computer Architecture
- Cache Lines
- Cache Hierarchy
- Memory Layout
- Memory Access
  
## Key-binds
- W - Move Forward / up on the menu
- S - Move Backwards / down on the menu
- A - Move Left
- D - Move Right
- Shift - Move Down
- Space - Move Up
- Enter - To open the scene type
- ESC - To close the scene or to close the game
- Mouse for looking around

## What I learned
Data-Oriented Design (DOD) showed much greater performance because of efficient use of cache Hierarchy and cache lines. This is achieved due to a greater focus on memory layout and memory access. In Object-Oriented Programming (OOP), data is essentially hidden and can become hard to access. 
This is further compounded upon by inheritance and polymorphism which cause scattered memory layout meaning that cache lines can’t be utilised efficiently.

By focusing on memory layout and memory access, DOD can efficiently use cache hierarchy and cache lines by not hiding data and not making data apart of the problem domain and instead focus on how data is transformed throughout the program. 
This dissertation also showed that Structure of Arrays (SOA) performed slightly better than Array of Structures (AOS). 
However, in another test that artificially filled AOS and SOA with garbage data, SOA performed much better since that data isn’t accessed since its contained in a separate array whereas that data is included in the record in the AOS implementation.

## How To Build
1. Install CMake (3.16.0 at minimum).
2. Open the folder and create a build folder.
3. Set the project folder as the source folder and the build folder as the output for the binaries.
4. Once it's built, open the build folder and open the CSC8503 .sln file.
5. You should now be able to run the project.

## Further Work
The main drawback of the dissertation is that multi-threading was not explored, this could also show how memory layout affects performance in a concurrent setting. 
A reduction of boolean values could also show the benefits of DOD when compared to OOP since there would be no padding in memory alignment. 
Other game systems such as AI and Networking could’ve also been explored in showing the difference between DOD and OOP since the dissertation mainly covers the physics and, to a lesser extent, the rendering parts of the engine.

## Video Link
https://www.youtube.com/watch?v=b8FbDDmuNlA
