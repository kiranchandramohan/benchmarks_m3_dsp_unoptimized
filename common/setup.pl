#!/usr/bin/perl
use File::Spec;

sub create_fn
{
        my $abs_path = File::Spec->rel2abs($0) ;
	my $abs_dir = `dirname $abs_path` ;
	chomp($abs_dir) ;

	my $sysbios_dir = $abs_dir ;
	my $common_dir = $sysbios_dir."/../common/" ;
	my $omx_dir = $sysbios_dir."/"."src/ti/examples/srvmgr/" ;
	my $omx_file = $sysbios_dir."/"."src/ti/examples/srvmgr/test_omx.c" ;
	my $dsp_firmware=$sysbios_dir."/src/ti/examples/srvmgr/ti_platform_omap4430_dsp/release/test_omx_dsp.xe64T" ;
	my $m3_firmware=$sysbios_dir."/src/ti/examples/srvmgr/ti_platform_omap4430_ipu/release/test_omx_ipu.xem3" ;
	my $template_file_name = $sysbios_dir."/"."template_test_omx.c" ;
	my $exp_name="exp" ;
	my $dir_name = $sysbios_dir."/".$exp_name ;
	my $m3_omx_file = $sysbios_dir."/"."m3_test_omx.c" ;
	my $dsp_omx_file = $sysbios_dir."/"."dsp_test_omx.c" ;
	system("rm -rf $dir_name") ;
	system("mkdir $dir_name") ;

	system("cp $common_dir/m3.c $omx_dir") ;
	system("cp $m3_omx_file $dir_name") ;
	my $m3_file_name = $dir_name."/"."m3_test_omx.c" ;
	system("cp $m3_file_name $omx_file ;cd $sysbios_dir; make -f Makefile_dsp clean ; make -f Makefile_m3 ; cd -") ;
	system("cp $m3_firmware $dir_name/") ;

	system("cp $common_dir/dsp.c $omx_dir") ;
	system("cp $dsp_omx_file $dir_name") ;
	my $dsp_file_name = $dir_name."/"."dsp_test_omx.c" ;
	system("cp $dsp_file_name $omx_file ;cd $sysbios_dir; make -f Makefile_m3 clean ; make -f Makefile_dsp ; cd -") ;
	system("cp $dsp_firmware $dir_name/") ;

	my $tar_file = "$exp_name.tgz" ;
	system("cd $sysbios_dir ; tar cvzf $tar_file $exp_name ; rm -rf $dir_name ; cd -") ;
}

$numArgs = $#ARGV + 1;
create_fn() ;

