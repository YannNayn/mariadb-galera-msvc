package My::Suite::TokuDB_bugs;
use File::Basename;
@ISA = qw(My::Suite);

#return "Not run for embedded server" if $::opt_embedded_server;
return "No TokuDB engine" unless $ENV{HA_TOKUDB_SO} or $::mysqld_variables{tokudb};
bless { };

