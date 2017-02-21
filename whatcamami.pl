#!/usr/bin/perl

$which = $ARGV[0];
$i = 0;

opendir(DIR, "/dev/");
@camera = grep{/video/}readdir DIR;
closedir(DIR);

foreach $camera (@camera){
    system("sudo udevadm info --query=all --name=/dev/$camera > tmp$i.txt");
    open(LOG,"tmp$i.txt");
    while(<LOG>){
        $usb[$i] = (split /_/, $_)[5] if(/ID_PATH_TAG/);
    }   
    close(LOG);
    $gear = $camera[$i] if ($usb[$i] == 5); 
    $gear =~ s/video//g;
    $turret = $camera[$i] if ($usb[$i] ==4);
    $turret =~ s/video//g; 
    $i++;
}

print "$gear" if($which eq "G");
print "$turret" if ($which eq "T");
