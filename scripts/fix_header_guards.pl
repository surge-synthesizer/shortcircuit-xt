#!/usr/bin/perl


use File::Find;
use File::Basename;

find(
    {
        wanted => \&findfiles,
    },
    'src'
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
        $hg =~ s:-:_:g;
        $hg =~ s:src:scxt_src:;
        $hg =~ s:src-ui:scxt_ui:;
        $hg = uc($hg);
        print "$f -> $hg\n";

        print "$f -> ${f}.bak\n";

        $q = basename($f);
        print "$q\n";
        open(IN, "<$q") || die "Cant open IN $!";
        open(OUT, "> ${q}.bak") || die "Cant open BAK $!";

        $tg = "notyet";
        $pragmaOnce = 0;
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
            elsif (m/#pragma\s*once/)
            {
                print OUT "#ifndef $hg\n#define $hg\n";
                $pragmaOnce = ff;
            }
            else
            {
                print OUT;
            }
        }
        if ($pragmaOnce)
        {
            print OUT "\n#endif // $hg\n";
        }
        close(IN);
        close(OUT);
        system("mv ${q}.bak ${q}");
    }
}
