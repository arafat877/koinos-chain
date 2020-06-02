#!/bin/bash

if [ -z "$KOINOS_VIRTUALENV" ]
then
   echo "$HOME/koinos/ve"
   echo "Using default KOINOS_VIRTUALENV location: $HOME/koinos/ve" >&2
else
   echo "$KOINOS_VIRTUALENV"
   echo "Using user-specified KOINOS_VIRTUALENV location:  $KOINOS_VIRTUALENV"
fi
