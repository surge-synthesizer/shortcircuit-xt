#!/usr/bin/perl


use File::Find;
use File::Basename;

find(
    {
        wanted => \&findfiles,
    },
    'src-next'
);
find(
    {
        wanted => \&findfiles,
    },
    'ui-next'
);

sub findfiles
{
    $f = $File::Find::name;
    if ($f =~ m/\.h$/)
    {
        #To search the files inside the directories

        $hg = $f;
        $hg =~ s:/:_:g;
        $hg =~ s:\.:_:g;
        $hg =~ s:src-next:scxt_src:;
        $hg =~ s:ui-next:scxt_ui:;
        $hg = uc($hg);
        print "$f -> $hg\n";

        print "$f -> ${f}.bak\n";

        $q = basename($f);
        print "$q\n";
        open(IN, "<$q") || die "Cant open IN $!";
        open(OUT, "> ${q}.bak") || die "Cant open BAK $!";

        $tg = "notyet";
        while(<IN>)
        {
            if (m/\#ifndef\s+(\S*)/)
            {
                $tg = $1;
                print OUT "#ifndef $hg\n";
            }
            elsif (m/\#define\s+${tg}/)
            {
                print OUT "#define $hg\n";
            }
            else
            {
                print OUT;
            }
        }
        close(IN);
        close(OUT);
        system("mv ${q}.bak ${q}");
    }
}