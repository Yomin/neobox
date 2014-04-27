# neobox

Userland - without X - replacement developed for the Openmoko Neo Freerunner.

## iod - IO daemon
Provides input events (touchscreen, buttons) for applications and
manages the application stack ie which app is active, has input
grabbed, etc.

## libtkbio
Provides a generic lib to hookup with the iod, utilize a keyboard layout and
draw simple graphics to the framebuffer.

## console
Simple application to provide keyboard input.

## slider
Simple application utilizing a slider to set a device value.

## menu
Application for starting other applications.
