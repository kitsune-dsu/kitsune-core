#!/bin/bash

target=$1

rsync -avzhPp doxygen/html $target
