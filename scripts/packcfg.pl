#!/usr/bin/perl
 
# Generates hashes and salts for Wraith
# http://wraith.botpack.net/wiki/PackConfig
 
use MIME::Base64;
use Digest::SHA;

# Configure options here or use command line switches (also check -h)

my $packname = "mypackName";
my $binarypass = "salted-SHA1";
my $dccprefix = ".";
my $owner = "YourNick";
my $password = "Password";
my @hub = (
  "hub1 hub1.domain.tld 1234",
  "hub2 hub2.domain.tld 1234"
);

my $basename = $0;
if ($0 =~ m|([^/\\]+?)$|) { $basename = $1 };
foreach $i (0 .. $#ARGV) {
  if ("$ARGV[$i]" eq "-h") {
    #my $hub_tmp;
    #foreach $j (0 .. $#hub) { my $sep = ""; if ($j < $#hub) { $sep=q(",") } $hub_tmp .= $hub[$j] . $sep };
    printf ('%2$sGenerate Wraith pack config %2$sUsage: %s -[h|n|p|d|o|u]%s %2$s', $0, "\n");
    printf ('%4s The options are as follows: %s', '', "\n");
    printf ('%6s -h %1$8s Help %s', '', "\n");
    printf ('%6s -n %1$8s PACKNAME: <%s>%s', '', $packname, "\n");
    printf ('%6s -b %1$8s BINARYPASS: <%s> %s', '', $binarypass, "\n");
    printf ('%6s -p %1$8s PASSWORD: <%s> %s', '', $password, "\n");
    printf ('%6s -d %1$8s DCCPREFIX: <%s> %s', '', $dccprefix, "\n");
    printf ('%6s -o %1$8s OWNER: <%s> %s', '', $owner, "\n");
    printf ('%6s -u %1$8s HUB(s): <"%s"> %s', '', join('","', @hub), "\n"x2);
    printf ('Options can also be set by changing variables in "%s" %s', $basename, "\n"x2);
    exit 0;
  } else {
    if ("$ARGV[$i]" =~ "-n") { shift; $packname = "$ARGV[$i]" };
    if ("$ARGV[$i]" =~ "-b") { shift; $binarypass = "$ARGV[$i]" };
    if ("$ARGV[$i]" =~ "-p") { shift; $password = "$ARGV[$i]" };
    if ("$ARGV[$i]" =~ "-d") { shift; $dccprefix = "$ARGV[$i]" };
    if ("$ARGV[$i]" =~ "-o") { shift; $owner = "$ARGV[$i]" };
    if ("$ARGV[$i]" =~ "-u") { shift; @hub=(); @hub = split ',', ("$ARGV[$i]") };
  }
}
if (@ARGV == 0) {
    printf ('%3$s/* Generated pack config from variables in "%s"%3$s * Run "%2$s -h" for help %3$s */%3$s', $basename, $0, "\n");
}
sub randstr {
	my $randsize = shift;
	my @alphanum = ('a'..'z', 'A'..'Z', 0..9);
	my $randstr = join '', map $alphanum[rand @alphanum], 0..$randsize;
	return $randstr;
}
sub genhash {
	my $name = shift;
	my $salt = shift;
	my $pass= shift;
        print "/* Password: $pass */" . "\n";
	print "$name +$salt\$" . Digest::SHA::sha1_hex( $salt . $pass ), '' . "\n";
}
print "\n" . "PACKNAME " . $packname . "\n";
genhash ("BINARYPASS", randstr(4), $binarypass);
print "DCCPREFIX " . $dccprefix . "\n";
genhash ("OWNER $owner", randstr(4), $password);
foreach (@hub) { print "HUB " . $_ . "\n"; };
print "SALT1 " . randstr(31) . "\n";
print "SALT2 " . randstr(16) . "\n\n";
