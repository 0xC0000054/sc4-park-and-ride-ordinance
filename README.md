# sc4-park-and-ride-ordinance

A DLL Plugin for SimCity 4 that adds a Park and Ride ordinance to the game.   

The ordinance allows the Park & Ride mode of the SimCity 4 traffic simulator to be 
enabled or disabled in-game. When the ordinance is enabled cars will be prevented from
reaching their destinations, which requires Sims to use other forms of transit.

The mod automatically adjusts `Travel type can reach destination` property in the traffic simulator tuning exemplar.
The second Boolean value of that property controls whether cars can reach their destination.
See the Network Addon Mod [Park and Ride](https://www.sc4nam.com/docs/feature-guides/the-nam-traffic-simulator/#park--ride) documentation for more details on this property.

## Technical Details

The mod works by loading the traffic simulator tuning exemplar and modifying the copy of the exemplar that the
game temporarily caches in-memory. The DLL then sends a message to the traffic simulator that makes it read the
new values from the cached exemplar.

## System Requirements

* Windows 10 or later

The plugin may work on Windows 7 or later with the [Microsoft Visual C++ 2022 x86 Redistribute](https://aka.ms/vs/17/release/vc_redist.x86.exe) installed, but I do not have the ability to test that.

## Troubleshooting

The plugin should write a `SC4ParknRideOrdinance.log` file in the same folder as the plugin.    
The log contains status information for the most recent run of the plugin.

# License

This project is licensed under the terms of the MIT License.    
See [LICENSE.txt](LICENSE.txt) for more information.

## 3rd party code

[gzcom-dll](https://github.com/nsgomez/gzcom-dll/tree/master) Located in the vendor folder, MIT License.    
[Windows Implementation Library](https://github.com/microsoft/wil) MIT License    

# Source Code

## Prerequisites

* Visual Studio 2022

## Building the plugin

* Open the solution in the `src` folder
* Update the post build events to copy the build output to you SimCity 4 application plugins folder.
* Build the solution

## Debugging the plugin

Visual Studio can be configured to launch SimCity 4 on the Debugging page of the project properties.
I configured the debugger to launch the game in a window with the following command line:    
`-intro:off -CPUcount:1 -w -CustomResolution:enabled -r1920x1080x32`

You may need to adjust the resolution for your screen.

## Source Code Layout

[ParknRideOrdinanceDllDirector.cpp](src/ParknRideOrdinanceDllDirector.cpp) is the main plugin file. It handles the setup that SC4 requires to load a DLL, and adding the ordinance into the game.

[ParknRideOrdinance.h](src/ParknRideOrdinance.h) defines the methods that the ordinance overrides for its custom behavior.    
[ParknRideOrdinance.cpp](src/ParknRideOrdinance.cpp) provides the implementation for the methods that the ordinance overrides.

[OrdinanceBase.cpp](src/OrdinanceBase.cpp) is the base class for `ParknRideOrdinance`. It handles the common logic for a custom ordinance.
[OrdinancePropertyHolder.cpp](src/OrdinancePropertyHolder.cpp) provides the game with a list of effects that should be applied when the ordinance is enabled.
The effects can include Mayor Rating boosts, Demand boosts, etc.
