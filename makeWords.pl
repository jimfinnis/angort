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
# %word name description..
# {
#     ...
# }
#
# is how you define a word. DO NOT put the curly brace on the same line
# as the word definition, and DO NOT put anything other than the brace
# on the first line of the function! 
# The variable 'a' is passed in as a pointer
# to Angort. The description typically starts with a stack picture,
# for example (a b -- a+b) for some kind of addition word.
#
# A new alternative is %wordargs, which allows you to specify a set
# of arguments and will generate the appropriate parameter popping
# code:
#
# %wordargs multiply nn (a b -- a*b)
#
# will generate code to pop arguments in to variables
# called p0 and p1, which are floats. Angort values are converted
# if appropriate and possible - check the generated code to see how.
#
# Special types can also be used. Assuming you have declared type
# objects called tFoo and tBar, you could do
#
# %wordargs combinefoobar nAB|foo,bar (n foo bar -- combination)
#
# to generate code to pull the variables into the types, which
# must have been registered with %type thus:
#
# %type name typeobject class
#
# e.g.
# %type foo tFoo FooObject
#
# Extra lines of description can be added after the %word or %wordargs,
# before the opening curly brace:
#
# %wordargs multiply nn (a b -- a*b)
# Multiples two thingies, leaving a thingy 
# on the stack.
# {
#     ...
# }
#
# If you have an initialisation function for this library, write it as
#
# %init
# {
#     ...
# }
#
# Similarly with the shutdown function:
#
# %shutdown
# {
#     ...
# }
#
# Binary operations (binops) can be defined with
#
# %binop lhstype operator rhstype
# {
#     ... do something with Value *lhs,*rhs
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
@binops=();
%types=();
%descs=();
$hasinit=0;
$hasshutdown=0;
$shared=0;

open(WORDSFILE,">words");
open(WORDSTEXFILE,">words.tex");

$waitingforfuncstart=0;
while(<>){
    chop;
    if(/^%name/){
        ($dummy,$libname)=split(/\s/,$_,2);
        $nsname = "_angortlib_ns_$libname";
        print WORDSTEXFILE "\\section{$libname}\n";
        print "namespace $nsname {\n";
    }elsif(/^%type/){
        ($dummy,$type,$typeobj,$class) = split(/\s/,$_,4);
        my @dat = ($typeobj,$class);
        $types{$type} = \@dat;
    }elsif(/^%wordargs/){ # new version: %wordargs myword nnAB|tFoo,tFish (n n a a --)
        defined($libname) || die "%name is required";
        ($dummy,$word,$args,$text)=split(/\s/,$_,4);
        if(index($args,"|")!=-1){
            ($args,$spec)=split(/\|/,$args,2);
            @specarr=split(/,/,$spec);
            map {
                my $t = $types{$_};
                if(!defined($t)){die "unknown special type: $_";}
            } @specarr;
            $hasspec=1;
        }else{
            $hasspec=0;
        }
            
        push(@list,$word);
        $descs{$word}=$text;
        $curword=$word;
        print WORDSFILE "$word,";
        print WORDSTEXFILE "\\index{$libname\\\$$word}\\subsection{$word}\n";
        print "static void _word__$word"."(angort::Angort *a)\n";
        # output a function def and opening curly bracket,
        # and the initial arg fetch
        print "{\nValue *_parms[".length($args)."];\n";
        print "a->popParams(_parms,\"$args\"";
        if($hasspec){
            @spectypeobjs = map {
                my $a = $types{$_};
                "&".$a->[0]}
            @specarr;
            print ",".join(",",@spectypeobjs);
        }
        print ");\n";
        # now convert the args
        for($i=0;$i<length($args);$i++){
            $c = substr($args,$i,1);
            if($c eq 'n'){
                print "float p$i = _parms[$i]->toFloat();\n"
            }elsif($c eq 'L'){
                print "long p$i = _parms[$i]->toLong();\n"
            }elsif($c eq 'b'){
                print "bool p$i = _parms[$i]->toBool();\n"
            }elsif($c eq 'i'){
                print "int p$i = _parms[$i]->toInt();\n"
            }elsif($c eq 'd'){
                print "double p$i = _parms[$i]->toDouble();\n"
            }elsif($c eq 'c' || $c eq 'v' || $c eq 'C'){
                print "Value * p$i = _parms[$i];\n"  # any value or callable
            } elsif($c eq 's' || $c eq 'S'){
                print "const StringBuffer &_sb$i = _parms[$i]->toString();\n";
                print "const char *p$i = _sb$i.get();\n";
            } elsif($c eq 'y'){
                print "const char *p$i=NULL;\n";
                print "if(!_parms[$i]->isNone()) {\n";
                print "const StringBuffer &_sb$i = _parms[$i]->toString();p$i=_sb$i.get();\n}";
            } elsif($c eq 'l'){
                print "ArrayList<Value> *p$i = Types::tList->get(_parms[$i]);\n";
            } elsif($c eq 'h'){
                print "Hash *p$i = Types::tHash->get(_parms[$i]);\n";
            } elsif($c =~ /^[A-B]/){
                my $idx = ord($c)-ord('A'); # get the index into the specials
                my $q = $specarr[$idx]; # get the special name
                my $tpo = $types{$q}->[0]; # get the typeobject
                my $cl = $types{$q}->[1]; # get the class
                print "$cl *p$i = $tpo.get(_parms[$i]);\n"
            } else {
                print "Value *p$i = _parms[$i];\n"
            }
        }
        $waitingforfuncstart=1;
    }elsif(/^%word/){
        defined($libname) || die "%name is required";
        ($dummy,$word,$text)=split(/\s/,$_,3);
        push(@list,$word);
        $descs{$word}=$text;
        $curword=$word;
        print WORDSFILE "$word,";
        print WORDSTEXFILE "\\index{$libname\\\$$word}\\subsection{$word}\n";
        # output a function def and opening curly bracket
        print "static void _word__$word"."(angort::Angort *a){\n";
        $waitingforfuncstart=1;
    }elsif($waitingforfuncstart && !/^{/){
            $descs{$curword}.="\\n".$_;
    }elsif(/^%binop/){
        defined($libname) || die "%name is required";
        ($dummy,$lhs,$opcode,$rhs)=split(/\s/,$_,4);
        $bname = "$lhs"."_$opcode"."_$rhs";
        print "//BINOP : $lhs - $opcode - $rhs = $bname\n";
        push(@binops,$bname);
        print "static void _binop__$bname(angort::Angort *a,angort::Value *lhs,angort::Value *rhs)\n";
    }elsif(/^%init/){
        print "static void __init__(angort::Angort *a)\n";
        $hasinit = 1;
    }elsif(/^%shutdown/){
        print "static void __shutdown__(angort::Angort *a)\n";
        $hasshutdown = 1;
    }elsif(/^%shared/){
        $shared=1;
        print "\n";
    } else {
        if($waitingforfuncstart){
            # output the descs and ignore the line, we assume it's
            # just a curly bracket
            $waitingforfuncstart=0;
            $t = $descs{$curword};
            # substitute nice things for LaTeX
            $t =~ s/\$/\\\$/g;
            $t =~ s/\\n/\n/g;
            $t =~ s/\&/\\&/g;
            $t =~ s/_/\\_/g;
            $t =~ s/\^/\\^/g;
            $t =~ s/_/\\_/g;
            # extract the first line.
            ($firstline,$t) = split(/\n/,$t,2);
            print WORDSTEXFILE $firstline."\n\n";
            print WORDSTEXFILE $t."\n";
        }else{
            print "$_\n";
        }
    }
}

defined($libname) || die "%name is required";

print "static angort::WordDef _wordlist_[]={\n";
foreach $v (@list) {
    $t = $descs{$v};
    # replace newlines with newline-space to make it look better in
    # ?? outputs. Escape chars.
    $t =~ s/\\n/\\n /g;
    # other replaces
    $t =~ s/\"/\\\"/g;
    print "    {\"$v\",\"$t\",_word__$v},\n";
}
print "    {NULL,NULL,NULL} };\n";

print "static angort::BinopDef _binoplist_[]={\n";
foreach $v (@binops) {
    ($lhs,$op,$rhs) = split(/\_/,$v,3);
    print "    {\"$lhs\", \"$rhs\", \"$op\",_binop__$v},\n";
}
print "    {NULL,NULL,NULL} };\n";

print "} // end namespace $nsname \n\n"; # close the namespace

if($hasinit){
    $initname="$nsname\::__init__";
} else {
    $initname = "NULL";
}
if($hasshutdown){
    $shutdownname="$nsname\::__shutdown__";
} else {
    $shutdownname = "NULL";
}


print <<EOT;
angort::LibraryDef _angortlib_$libname = {
    "$libname",
    $nsname\::_wordlist_,
    $nsname\::_binoplist_,
    $initname,$shutdownname
};
EOT


if($shared){
    print <<EOT;
extern "C" angort::LibraryDef *init(angort::Angort *api){
    return &_angortlib_$libname;
}   
EOT
}
