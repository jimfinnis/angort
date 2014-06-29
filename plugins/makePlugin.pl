#!/usr/bin/perl
#
# Generate a list of shared library plugin word definitions,
# as opposed to makeWords.pl, which generates words for an Angort module
# to be compiled into Angort itself.
# This takes a CPP file with the following markup:
#
# %plugin name 
#
# is the name of the output structure (which will have _wordlist_...
# prepended) and
# 
# %word name paramcount description..
# {
#     ...
# }
#
# is how you define a word. Each word will take two parameters,
# both PluginValue pointers.
# * res points to the value into which any result should be stored
# * params is an array of the parameters.
#

@list=();
%descs=();
%counts=();

open(WORDSFILE,">words");

while(<>){
    chop;
    if(/^%plugin/){
        ($dummy,$listname)=split(/\s/,$_,2);
        print "\n"; # to make sure the line numbers are still in sync
    }elsif(/^%word/){
        ($dummy,$word,$argcount,$text)=split(/\s/,$_,4);
        push(@list,$word);
        $descs{$word}=$text;
        $counts{$word}=int($argcount);
        print WORDSFILE "$word,";
        print "static void _plugin_$word"."(PluginValue *res,PluginValue *params)\n";
    }elsif(/^%init/){
        print "static void _plugininit_()\n";
    } else {
        print "$_\n";
    }
}

print "\n\n";
print "static PluginFunc _pluginfuncs_ []={\n";
foreach $v (@list) {
    $t = $descs{$v};
    $ct = $counts{$v};
    print "    {\"$v\",_plugin_$v,$ct,\"$t\"},\n";
}
print "    {NULL,NULL,-1,NULL}\n};\n";

print <<EOT;
static PluginInfo info = {
    "$listname",_pluginfuncs_
};

extern "C" PluginInfo *init(){
    _plugininit_();
    return &info;
}
EOT


