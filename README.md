# Kinect 360 Virtual Camera.
This little project is a revival of the Original [KinectCam](https://github.com/wildbillcat/KinectCam) which is a fork of [VCam](https://github.com/roman380/tmhare.mvps.org-vcam) that is archived by roman380.

*⚠️ This is just a personal project to make use of the Kinect v1 as a Camera and a tracking solution for my own needs, and has been made public incase other people would like to to use it.*

Though any help/fixing problems will be greatly appreciated, ty.

## Prerequisites
- Kinect 1.8 SDK
- Kinect SDK Developers Toolkit (for turning off the IR sensor properly)

## Issues
- The IR Camera will not turn off on its own, you will have to run one of the kinect samples from the Developer Toolkit that uses the IR to turn it off.
- There are times where the camera will not work/hangs the SDK Samples (to turn off the IR for example).
  - fix: Unregistering and replugging then reregistering will usually do the trick.
- Forcefully closing applications causes unintended behaviour, the fix here is the same as above.
- Forecfully closing applications does not uninitialize the Camera properly, and doesnt return to its shutdown angle.

## How to build
- Have CMake 3.8.
- Pick a branch you'd like to use and clone it on your machine.
  - (NV12, RGB, RGB-RGBSENSOR) - Branches that dont specify what sensor its using means its using the IR sensor.
- Build project.

## How to use (Run as admin)
- *(Run Unregister.cmd incase you are registering a new branch, eg: NV12, RGB, RGB-RGBSENSOR)*
- Run Register.cmd

# TODO (currently not a priority)
- Create a config file for the shutdown/open angles.
- Merge all the camera types together into one config so nobody needs to go through unregistering then registering to use other sensors.
- Create releases for each branch so others dont have to build the binaries themselves (i cant be bothered rn)
