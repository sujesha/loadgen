#!/usr/bin/perl
#
# Script to log xentop batch statistics with timestamp. Note that it
# only works with xentop in batch mode (obviously).
#
# contact: Sujesha Sudevalayam
#
# syntax:
#       perl xentop-with-timestamp.pl [$intervalsecs=5] [$num_samples=0]
#
# Original src: xenstat.pl
# Note that this is only a wrapper for the xentop tool, it will print
# a single line output with all domains info concatenated. Only the
# interval and number of samples can be specified on the command-line.
#
#
use strict;
use POSIX qw(floor);
use Time::HiRes qw(sleep time);

my $XENTOP = '/usr/sbin/xentop';
my $XM = '/usr/sbin/xm';

unless(-e $XENTOP) {
        print "$XENTOP not found\n";
        die;
}

###############
# subroutines
###############

# trims leading and trailing whitespace
sub trim($)
{
  my $string = shift;
  $string =~ s/^\s+//;
  $string =~ s/\s+$//;
  return $string;
}

###########################
# Parsing parameters
###########################
#Retrieving first param from command-line
my $intervalsecs = shift || 5;
if ($intervalsecs < 1) {$intervalsecs = 1;}

#Retrieving second param from command-line
my $num_samples = shift || 0;


#Temporary interval period for pinging xentop
my $interval = 0.1;


###############
# Setup vars
###############
my $loop = 0;

###########################
# Get statistics using xentop
###########################
while ( ++$loop ) { # loop forever
   my $buf = '';
   my $count = 0;
   my @result = split(/\n/, `$XENTOP -b -i 2 -d$interval`);
  exit if ($num_samples && $loop > $num_samples);
  $interval = $intervalsecs;
  # remove the first line
  shift(@result);
  shift(@result) while @result && $result[0] !~ /^[\t ]+NAME/;
  shift(@result);

   my $curtime = time();

   my ($sec,$min,$hour,$mday,$mon,$year,$wday,
   $yday,$isdst)=localtime($curtime);
   $buf = sprintf "%4d-%02d-%02d %02d:%02d:%02d",
                  $year+1900,$mon+1,$mday,$hour,$min,$sec;
   print($buf);
   print(@result);
   print("\n");
}


