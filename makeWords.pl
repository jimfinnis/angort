#!/usr/bin/perl
#
# Generate a list of word definitions (WordDef structures)
# from a CPP file with the following markup:
#
# %name name 
#
# is the name of the output structure (which will have _wordlist_...
# prepended) and
# 
# %word name
# {
#     ...
# }
#
# is how you define a word. The variable 'a' is passed in as a pointer
# to Angort.
#
# If you have an initialisation function for this library, write it as
#
# %init
# {
#     ...
# }
# 
# A structure called will be defined, which can
# be imported into Angort using the RegisterLibrary() method.
#
# If you require a shared library, use the %shared directive - this
# will add an init() function which will link the library into
# Angort on load.
#
# Be EXTREMELY CAREFUL with namespaces here - do not switch namespaces
# AFTER the %name statement; weird things tend to happen.
#
# Input and output for this script are stdin/out.
#

@list=();
%descs=();
$hasinit=0;
$shared=0;

open(WORDSFILE,">words");

while(<>){
    chop;
    if(/^%name/){
        ($dummy,$libname)=split(/\s/,$_,2);
        $nsname = "_angortlib_ns_$libname";
        print "namespace $nsname {\n";
    }elsif(/^%word/){
        defined($libname) || die "%name is required";
        ($dummy,$word,$text)=split(/\s/,$_,3);
        push(@list,$word);
        $descs{$word}=$text;
        print WORDSFILE "$word,";
        print "static void _word__$word"."(angort::Angort *a)\n";
    }elsif(/^%init/){
        print "static void __init__(angort::Angort *a)\n";
        $hasinit = 1;
    }elsif(/^%shared/){
        $shared=1;
        print "\n";
    } else {
        print "$_\n";
    }
}

defined($libname) || die "%name is required";

print "static angort::WordDef _wordlist_[]={\n";

foreach $v (@list) {
    $t = $descs{$v};
    print "    {\"$v\",\"$t\",_word__$v},\n";
}
print "    {NULL,NULL} };\n";
print "} // end namespace $nsname \n\n"; # close the namespace

if($hasinit){
    $initname="$nsname\::__init__";
} else {
    $initname = "NULL";
}


print <<EOT;
angort::LibraryDef _angortlib_$libname = {
    "$libname",
    $nsname\::_wordlist_,
    $initname
};
EOT


if($shared){
    print <<EOT;
extern "C" angort::LibraryDef *init(angort::Angort *api){
    return &_angortlib_$libname;
}   
EOT
}
