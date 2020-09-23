#!/usr/bin/perl

use strict;
use warnings;

my $description = `git describe --long --always --dirty 2>/dev/null`;

my ($version, $delta, $tag, $dirty);

if ($description =~ /^v(.*)-(\d+)-(g[[:xdigit:]]+)(-dirty)?$/) {
  ($version, $delta, $tag, $dirty) = ($1, $2 + 0, $3, $4);
}
else {
  ($version, $delta, $tag, $dirty) = ('0.0.1-alpha.1', 0, '', '');
}

$delta++, $dirty = '' if $dirty;

$version =~ /^(?P<major>0|[1-9]\d*)\.(?P<minor>0|[1-9]\d*)\.(?P<patch>0|[1-9]\d*)(?:-(?P<prerelease>(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*)(?:\.(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*))*))?(?:\+(?P<buildmetadata>[0-9a-zA-Z-]+(?:\.[0-9a-zA-Z-]+)*))?$/ or die "unable to parse '$version'\n";

my ($major, $minor, $patch, $prerelease) =
  @+{'major', 'minor', 'patch', 'prerelease'};

# The prerelease may be a dot-separated tuple
my @prerelease = split /\./, ($prerelease //= '');

# If the prerelease is not empty, increment the last component of it.
# If the last component is not an integer, append a new integer.
# Otherwise, increment the patch level.

if (@prerelease) {
  push @prerelease, 1 unless $prerelease[-1] =~ /^(\d+)$/;
  $prerelease[-1] += $delta;
  $prerelease = join('.', @prerelease);
}
else {
  $patch += $delta;
}

$delta = 0;

$version  = "$major.$minor.$patch";
$version .= "-$prerelease" if $prerelease;

print "$version\n";
