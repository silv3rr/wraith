#! /usr/bin/perl
#
# Generates hashes and salts for Wraith Botpack
# Change <YourNick> and <Password> to your own
#
use MIME::Base64;
use Digest::SHA;
sub randstr {
	my $randsize = shift;
	my @alphanum = ('a'..'z', 'A'..'Z', 0..9);
	my $randstr = join '', map $alphanum[rand @alphanum], 0..$randsize;
	return $randstr;
}
sub genhash {
	my $name = shift;
	my $salt = shift;
	my $pw = shift;
	print "# $pw\n";
	print "$name +$salt\$" . Digest::SHA::sha1_hex( $salt . $pw ), '' . "\n";
}

genhash ("BINARYPASS", randstr(4), '<Password>');
genhash ("OWNER <YourNick>", randstr(4), '<Password>');
print "SALT1 " . randstr(31). "\n";
print "SALT2 " . randstr(16). "\n";
