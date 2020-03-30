# Godot-Navigation-Lite
Current version: 0.6

Godot plug&amp;play implementation of [detour library](https://github.com/recastnavigation/recastnavigation). It allows user to have multiple navigation meshes, fast cached obstacles, and realtime updates on the navigation meshes.

Because Godot's implementation of recast lacks dynamic/realtime navigation mesh updates this plugin comes handy to developers that want simple yet performat way of doing navigation mesh updates.

This addon does not implement all the features of Detour/Recast library (yet), but it allows user to add or remove components to terrain which influence navigation. Current features are:

* Fast adding and removing static objects (It uses tiled navigation mesh)
* Having multiple navigation meshes for agents of different sizes and with separate parameters (e.g. a navigation mesh for cars, people, vehicles that aren't stopped by trees).  
* Adding and removing very fast dynamic obstacles (cached obstacles - check tilecache implementation in detour library) 
* More to come

Currently it's suited for fixed size levels. It was meant for usage with RTS games, but should be more than capable of being used by any genre as long as the size of the level does not change.

Usage
===============

## Installation
Currently supports Linux and Windows platforms (If anyone is willing to build for OSx please contact me).

Installation is the same as any other Godot addon.
Download the addon and copy it to addons folder. Enable it in project settings and restart your project.

## Setup

Usage is very similar to the one in vanilla Godot. After setting up the addon, add a new node named `DetourNavigation`. This is the root navigation node, and it should be the parent of all the nodes (like with regular Godot node), that you want to include in your navigation. Then click on that node, and on the top menu, you will see Navigation manager button.

Choose create navigation mesh or cached navigation mesh, if you require fast dynamic obstacles, besides the static ones.

Once you select the wanted navigation mesh, a new node will be added. Like with default godot navigation mesh, you have to select it and bake it. Once you do this, be sure to save the project (otherwise you will have to bake it again).

When it's baked, you can start your scene and call find_path function on the navigation mesh node. It will return an array of vertices (same as Godot navigation mesh).

## Important notes
* Addon nodes like DetourNavigation, DetourNavigationMesh and CachedDetourNavigationMesh have automatically gdnative scripts added. Do not change them or you will lose functionality. Add child nodes with extra logic instead.
* Detour navigation always has to be direct parent to navigation mesh nodes and indirect parent to any of the geometries you want to use for building meshes. Navigation meshes do not have to be parents.
* Do not remove navigation mesh parameters from navigation mesh nodes
* When baking, you have to save the scene, otherwise you'll have to bake it again next time.
* Baked resources stay in .navcache folder in the root of the project
* You can delete the cached files by clearing the navigation mesh in menu, or by manually deleting those files.
* Dynamic objects will work when you start the scene, in editor they're asleep.

Documentation
===============

## Recast parameters


## Functions

1. `find_path(Vector3 start, Vector3 end)`   
Defined on `DetourNavigationMesh` and `CachedDetourNavigationMesh`
Returns an array of Vector3 values that are points from start to end. If there is no possible path it returns null. If end or start is out of bounds it will return the path to the nearest point on the navmesh.

2. `move_dynamic_obstacle(var collision_shape_instance_id, Vector3 new_position)`   
*Not yet implemented*

## Contributing

Feel free to open issues about bugs, create pull requests that fix them or add functionalities.

Setting up the addon is pretty simple - you have to follow the same tutorial as described in [Godot official GDNative tutorial](https://docs.godotengine.org/en/3.1/tutorials/plugins/gdnative/gdnative-cpp-example.html). Setup is the same, you have to init the cpp bindings folder and then run `scons platform="windows"` or `linux` if you use linux. Binaries will be compiled in addon folder that is the part of demo project. To test it run change folder to `godot_project` and run `godot main.tscn -e`.



## License

Godot Navigation Lite is under MIT license. 

Creator: Miloš Lukić

Special thanks to [@slapin](https://github.com/slapin) on whose implementation of recast I based the navigation mesh generation logic. His implementation is available on his godot branch, where he implemented it as a module.

And also thanks to [@sheepandshepherd](https://github.com/sheepandshepherd) for helping with numerous Godot and C++ issues and bugs.