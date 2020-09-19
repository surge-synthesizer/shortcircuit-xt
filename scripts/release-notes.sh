#!/bin/bash

cat <<- EOH
# Automatically generated Secret Project 3 release

This release is automatically generated every time we push Secret Project 3
to master. As such it is the latest version of our code and may be
unstable, unusable, unsuitable for human consumption, and so on.

The build date and most recent commits are:

EOH
date
echo ""
echo "Most recent commits:" 
echo ""
git log --pretty=oneline | head -5
