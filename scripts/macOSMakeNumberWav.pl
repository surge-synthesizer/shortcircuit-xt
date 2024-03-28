#!/usr/bin/perl

for ($i=1; $i<10; $i++)
{
   system("say -o $i.aiff -v Allison $i");
   system("sox $i.aiff $i.wav");
   system("rm $i.aiff");
}
