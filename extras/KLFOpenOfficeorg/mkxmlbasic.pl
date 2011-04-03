#!/usr/bin/perl -w

use XML::Writer;
use IO::Handle;

STDOUT->autoflush(1);

my $xmlwriter = new XML::Writer;

push @all, $_  while (<>);

$allscript = join("", @all);

print <<EOHEADER;
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE script:module PUBLIC "-//OpenOffice.org//DTD OfficeDocument 1.0//EN" "module.dtd">
EOHEADER

$xmlwriter->startTag("script:module", "xmlns:script"=>"http://openoffice.org/2000/script",
		     "script:name"=>"KLFOOoMain", "script:language"=>"StarBasic");
$xmlwriter->characters($allscript);
$xmlwriter->endTag();

