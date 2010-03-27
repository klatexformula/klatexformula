#!/usr/bin/perl -w

use strict;


my ($target, $oldlib, $newrellib) = @ARGV;

my $otoolout = `otool -L "$target"`;
my @otoolout = split(/\n/i, $otoolout);
print "Got lines: \n-->".join("\n-->", @otoolout)."\n";

my $libline = m|^\s*(\S+(\Q$oldlib.framework\E\S+))\s|;

my @matches = grep /$libline/, @otoolout;
foreach $line (@matches) {
  if ($line =~ /$libline/) {
    my $oldfulllib = $1;
    my $newfulllib = '@'."executable_path/../Frameworks/$2";
    system "install_name_tool -change '$oldfulllib' '$newfulllib' '$target'";
  }
}

