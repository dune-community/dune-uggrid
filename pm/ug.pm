package ug;
use strict;
use vars qw (@ISA @EXPORT @EXPORT_OK %EXPORT_TAGS $VERSION);

use Exporter;
$VERSION = 1.0;
@ISA = qw(Exporter);

@EXPORT = qw(ug float tmpfile);
@EXPORT_OK = qw();
%EXPORT_TAGS = qw();

##############################################
## source 
##############################################

use IPC::Open2;
use IO::Handle;
use IO::File;
use POSIX qw(tmpnam);
BEGIN
{
	my $debug=0;
	my $end='';
	STDOUT->autoflush(1);
	sub debug
	{
		$debug=$_[0];
		if ($debug)
		{
			open(DEBUG,">debug.scr");
			DEBUG->autoflush(1);
		}
	}
	sub submit
	{
		if ($_[0]=~/quit/) {die "ERROR: quit command is blocked, use 'end'\n";}
	    print IN $_[0];
		if ($debug) 
		{ 
			$_[0]=~/(.*)/;
			print DEBUG "$1;\n"; 
		}
	}
	sub tmpfile
	{
        my $name;
        do { $name=tmpnam(); } until IO::File->new($name,O_RDWR|O_CREAT|O_EXCL);
        $end.="unlink('$name');";
        return $name;
	}
	END
	{
		eval $end;
		close(DEBUG);
	}
	sub out
	{
		my ($line,$ret,$error);
		$ret=""; $error=0;
		while($line=<OUT>)
		{
			if ($line=~/ERROR/) {$error=1;}
			if ($line=~/^EOO$/) {last;}
			if ($_[0] || $error) {print $line;}
			$ret.=$line;
		}
		if ($error) {die "ug aborted due to ERROR\n";}
		return $ret;
	}
}
sub set
{
	print IN "set $_[0]\n";
    return split /[=\s]+/,out(0);
}

sub ug
{
	my (@in,$i,$cmd,$print,$command,$ui,$stat,%argv,$dummy);

	# cancel trailing white spaces
	@in=@_;
	for ($i=0; $i<@in; $i++) { $in[$i]=~s/\s*$//g; }

	# basic check
	if (@in<=0) 
	{
		die "ERROR: provide ug command\n";
	} 
	if ($in[0] eq '') { return; }
	if ($in[0]=~/^\s*\#/) { return; }

	# detect internal print
	$command=$in[0]; 
	$print=0;
	if ($command=~/^\s*print\s+/) 
	{
		$print=1; 
		$command=~s/\s*^print\s+//g;
		if ($command eq "") {die "ERROR: print must come with ug command\n";}
	}

	# check for I/O channels
	if ($command ne "start")
	{
		1==stat IN and 1==stat OUT or die "ERROR in '$command': IN/OUT channel missing\n";
		1==stat IN or die "ERROR in '$command': IN channel missing\n";
		1==stat OUT or die "ERROR in '$command': OUT channel missing\n";
	}

	# scan arguments (used for some commands)
	@in%2==1 or die "odd number of arguments provided with command '$in[0]'\n";
	($dummy,%argv)=@in; 
	SWITCH:
	{
		# debug
		if ($command eq "debug")
		{
			if (@in!=3 || ($in[2]!=0 && $in[2]!=1))
            {
                die "ERROR: provide [0|1] with 'debug'\n";
            }
			debug $in[2];
			return;
		}

		# command 'end'
		if ($command eq "end")
		{
			if(@in!=1)
        	{   
        	    die "ERROR: don't provide any option with 'end'\n";
        	} 
        	print IN "quit\n";
        	close(IN);
        	close(OUT);
			return;
		}

		# command 'set'
		if ($command eq "set")
		{
			if(@in!=2)
            {   
                die "ERROR: provide one option with 'set'\n";
            }
			print IN "set $in[1]\n";
			return split /[=\s]+/,out($print);
		}

		# command 'start'
		if ($command eq "start")
		{
			if(@in!=3)
        	{   
        	    die 'ERROR: usage: ug "start", "p"=>"program";'."\n";
        	} 
			
			$ui="-ui c";
        	open2(*OUT,*IN,"$argv{'p'} $ui -perl");
			IN->autoflush(1); OUT->autoflush(1);
			return out($print);
		}
	
		# command 'ug' 
		if ($command eq "ug")
        {
			if(@in!=2)
            {
                die "ERROR: command 'ug' must come with one argument\n\n";
            }
			submit "$in[1]\n";
			return out($print);
		}

		# std command
		if (@in%2==1) {$cmd="$command "; $i=1;}
		else {$cmd="$command $in[1] "; $i=2;}
		for (;$i<@in;$i+=2)
		{
			$cmd.='$'."$in[$i] $in[$i+1] ";
		}
		$cmd.="\n";
		submit $cmd;
		return out($print);
	}
}
sub float
{
	my $real='[+-]?\d+\.?\d*[eE]?[+-]?\d+|[+-]?\d*\.?\d+[eE]?[+-]?\d+|[+-]?\d+';
	my (@list,$f,$s,$in);

	if (@_==1) { @list=grep /$real/,split /($real)/,$_[0]; }
	elsif (@_==2) 
	{
		$in=' '.$_[1];
		($f,$s)=split /$_[0]/,$in,2;
		@list=grep /$real/,split /($real)/,$s;
	}
	else
	{
		$in=' '.$_[2];
        ($f,$s)=split /$_[0]/,$in,2;
        @list=grep /$real/,split /($real)/,$s;
		if ($_[1]>=@list || $_[1]<0) { return undef;}
		for ($s=0; $s<$_[1]; $s++) { shift @list; } 
    }
	return wantarray ? @list : $list[0];
}

1;




