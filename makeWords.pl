#!/usr/bin/perl
#
# Generate a list of word definitions (AngortWordDef structures)
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
# Input and output are stdin/out.

@list=();
%descs=();

$listname="default";

while(<>){
    chop;
    if(/^%name/){
        ($dummy,$listname)=split(/\s/,$_,2);
        print "\n"; # to make sure the line numbers are still in sync
    }elsif(/^%word/){
        ($dummy,$word,$text)=split(/\s/,$_,3);
        push(@list,$word);
        $descs{$word}=$text;
        print "static void _word__$word"."(Angort *a)\n";
    } else {
        print "$_\n";
    }
}

print "\n\n";
print "static const char *_modulename_$listname = \"$listname\";\n";
print "AngortWordDef _wordlist_$listname"."[]={\n";
foreach $v (@list) {
    $t = $descs{$v};
    print "    {_modulename_$listname, \"$v\",\"$t\",_word__$v},\n";
}
print "    {NULL,NULL} };\n";
