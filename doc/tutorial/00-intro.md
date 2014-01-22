Introduction to Kitsune
=======================

Kitsune is a free, functional, usable dynamic updating framework. Using Kitsune, you can upgrade your program at runtime, with no overhead and minimal downtime.

What is Dynamic Software Updating
---------------------------------

Kitsune is designed to make DSU just another program feature. Much like a music player might use an mDNS/Zeroconf library to implement local-network sharing, a program can use Kitsune as a DSU library to implement DSU (or any other program transformation).

Nuts and Bolts: Implementing DSU with Kitsune
---------------------------------------------

Generally speaking, adding DSU support to your program requires the following steps:

1. Updating your build scripts to produce a shared-object that can be run by the Kitsune framework.
2. Adding update points, code to return to update points, and code that prevents re-initialization of state We call these *control* changes.
3. Adding automigration or manual migration statements, annotating functions that require local variable tracking. We call these *migration* changes.
4. Writing transformers that can convert between old and new version types and state. We call these *transformer* changes, and most of them involve our tool `xfgen`. `xfgen` handles many obvious types of transformers, and allows you to use an intuitive language to specify the rest of the transformation code. `xfgen` can also incorporate raw C code, allowing you to perform arbitrary transformations during updating.

This tutorial will walk you through making all of these changes to simple programs. We encourage you to follow along by doing the exercises associated with each chapter.

