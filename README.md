# Godot-Navigation-Lite
Current version: 0.9 Beta


![Image not found][main]

Godot plug&amp;play implementation of [detour library](https://github.com/recastnavigation/recastnavigation). It allows user to build multiple navmeshes from static objects (physics objects) for different agent sizes, it also features fast cached obstacles, and realtime updates on the navigation meshes.

Because Godot's implementation of recast lacks dynamic/realtime navigation mesh updates this plugin comes handy to developers that want simple yet performat way of doing navigation mesh updates.

This addon does not implement all the features of Detour/Recast library (yet), but it allows user to add or remove components to terrain which influence navigation. Current features are:

* Fast adding and removing static objects (It uses tiled navigation mesh)
* Having multiple navigation meshes for agents of different sizes and with separate parameters (e.g. a navigation mesh for cars, people, vehicles that aren't stopped by trees).  
* Adding and removing very fast dynamic obstacles (cached obstacles - check tilecache implementation in detour library) 
* More to come

Currently it's suited for fixed size levels. It was tested on a RTS game, but should be more than capable of being used by any genre as long as the size of the level does not change.


## Installation
Currently supports Linux and Windows platforms (If anyone is willing to build for OSx please contact me).

[Download GodotNavigationLite](https://github.com/MilosLukic/Godot-Navigation-Lite/archive/master.zip)

- Create ```addons``` folder in the root of your Godot project (if you don't have it yet)
- Download the addon, and copy the folder ```godot_project/addons/godot-navigation-lite``` from the zip to your ```addons``` folder, so you have ```addons/godot-navigation-lite``` in your project.
-  Go to project settings, click on Plugins tab and change the status of GodotNavigationLite to active.

## Setup

Usage is very similar to the one in vanilla Godot. After setting up the addon, add a new node named `DetourNavigation`. This is the root navigation node, and it should be the parent of all the nodes (like with regular Godot node), that you want to include in your navigation. Then click on that node, and on the top menu, you will see Navigation manager button.
 
![Image not found][ss1]

Choose create navigation mesh or cached navigation mesh, if you require fast dynamic obstacles, besides the static ones.

![Image not found][ss3]

Once you select the wanted navigation mesh, a new node will be added. Like with default godot navigation mesh, you have to select it and bake it. Once you do this, be sure to save the project (otherwise you will have to bake it again).

![Image not found][ss4]

When it's baked, you can start your scene and call find_path function on the navigation mesh node (see documentation below). It will return an array of flags and vertices (same as Godot navigation mesh).


![Image not found][ss6]

If you want to see the navigation mesh in game, just enable the Godot's "debug navigation" option in the debug menu (note - navigation will run slower as it has to create debug mesh every time).

## Important notes
* Make sure no two physics bodies (that are used to generate navmesh) share the same location.
* Addon nodes like DetourNavigation, DetourNavigationMesh and CachedDetourNavigationMesh have automatically gdnative scripts added. Do not change them or you will lose functionality. Add child nodes with extra logic instead.
* Detour navigation always has to be direct parent to navigation mesh nodes and indirect parent to any of the geometries you want to use for building meshes. Navigation meshes do not have to be parents.
* Do not remove navigation mesh parameters from navigation mesh nodes
* When baking, you have to save the scene, otherwise you'll have to bake it again next time.
* Dynamic objects will work when you start the scene, in editor they're asleep.

## Documentation

![Image not found][ss5]

### Class `DetourNavigation`
- `auto_add_remove_objects`   
if checked, a signal is connected, so every time a node enters it is checked whether it is collision shape and then it's added to navigation meshes based on their collision layer settings. Same goes for deleting nodes.
- `add_collision_shape(CollisionShape collision_shape)`   
Adds the collision shape to all `DetourNavigationMesh` and `DetourNavigationMeshCached` children, if collision shapes parent (static object) is in the layer that is covered by `collision_mask` parameter in Navigation meshes.
- `remove_collision_shape(CollisionShape collision_shape)`    
Removes the collision shapes under the same conditions as add function.

- `add_cached_collision_shape(CollisionShape collision_shape)`
Adds fast obstacle to `DetourNavigationMeshCached` children if collision shapes parent (any physics object) is in the layer that is  covered by  `dynamic_objects_collision_mask` parameter.

- `remove_cached_collision_shape(CollisionShape collision_shape)`   
Removes the collision shapes under the same conditions as add function.

### Class `DetourNavigationMesh`

- `find_path(Vector3 start, Vector3 end)`   
Returns a dictionary with two arrays, first is a vector of Vector3 values that are points from start to end, second is an Array of flags which is currently not usable. If there is no possible path it returns null. If end or start is out of bounds it will return the path to the nearest point on the navmesh.

- `bake_navmesh()`   
Creates navigation mesh and saves it with all helper meshes to a scene, so the next time it's loaded fast from the scene file.

- `clear_navmesh()`   


### Class `DetourNavigationMeshCached`
This class includes all methods and properties form `DetourNavigationMesh`



## Contributing

Feel free to open issues about bugs, create pull requests that fix them or add functionalities.

Setting up the addon is pretty simple - you have to follow the same tutorial as described in [Godot official GDNative tutorial](https://docs.godotengine.org/en/3.1/tutorials/plugins/gdnative/gdnative-cpp-example.html). Setup is the same, you have to init the cpp bindings folder and then run `scons platform="windows"` or `linux` if you use linux. Binaries will be compiled in addon folder that is the part of demo project. To test it run change folder to `godot_project` and run `godot main.tscn -e`.
```bash
cd godot-navigation-lite
git submodule update --init --recursive
godot --gdnative-generate-json-api api.json

cd godot-cpp
scons platform=<platform> generate_bindings=yes use_custom_api_file=yes custom_api_file=../api.json bits=64 target="debug"

cd ..
scons platform=<platform> # Add target="release" once you want to release it
```
Build also for release once you're ready by replacing the target="debug" for "release". I had to remove -g from scons file in godot-cpp (when using linux) to reduce the .so file size in linux for the release version (from 78MB to cca 4.5MB).


## License

Godot Navigation Lite is under MIT license. 

Creator: Miloš Lukić

Special thanks to [@slapin](https://github.com/slapin), I based the navigation mesh generation logic on his implementation of Detour/Recast to Godot module. His module implementation is available on his godot branch.

And also thanks to [@sheepandshepherd](https://github.com/sheepandshepherd) for helping with numerous Godot and C++ issues and bugs.


[ss1]: https://github.com/MilosLukic/Godot-Navigation-Lite/blob/master/documentation_assets/ss1.png "Logo Title Text 2"

[ss3]: https://github.com/MilosLukic/Godot-Navigation-Lite/blob/master/documentation_assets/ss3.png "Logo Title Text 2"

[ss4]: https://github.com/MilosLukic/Godot-Navigation-Lite/blob/master/documentation_assets/ss4.png "Logo Title Text 2"
[ss5]: https://github.com/MilosLukic/Godot-Navigation-Lite/blob/master/documentation_assets/ss5.png "Logo Title Text 2"

[main]: https://i.imgflip.com/3vcsox.gif