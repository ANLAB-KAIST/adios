HXCOMM Use DEFHEADING() to define headings in both help text and texi
HXCOMM Text between STEXI and ETEXI are copied to texi version and
HXCOMM discarded from C version
HXCOMM DEF(option, HAS_ARG/0, opt_enum, opt_help, arch_mask) is used to
HXCOMM construct option structures, enums and help message for specified
HXCOMM architectures.
HXCOMM HXCOMM can be used for comments, discarded from both texi and C

DEFHEADING(Standard options:)
STEXI
@table @option
ETEXI

DEF("help", 0, QEMU_OPTION_h,
    "-h or -help     display this help and exit\n", QEMU_ARCH_ALL)
STEXI
@item -h
@findex -h
Display help and exit
ETEXI

DEF("version", 0, QEMU_OPTION_version,
    "-version        display version information and exit\n", QEMU_ARCH_ALL)
STEXI
@item -version
@findex -version
Display version information and exit
ETEXI

DEF("machine", HAS_ARG, QEMU_OPTION_machine, \
    "-machine [type=]name[,prop[=value][,...]]\n"
    "                selects emulated machine ('-machine help' for list)\n"
    "                property accel=accel1[:accel2[:...]] selects accelerator\n"
    "                supported accelerators are kvm, xen, hax, hvf, whpx or tcg (default: tcg)\n"
    "                kernel_irqchip=on|off|split controls accelerated irqchip support (default=off)\n"
    "                vmport=on|off|auto controls emulation of vmport (default: auto)\n"
    "                kvm_shadow_mem=size of KVM shadow MMU in bytes\n"
    "                dump-guest-core=on|off include guest memory in a core dump (default=on)\n"
    "                mem-merge=on|off controls memory merge support (default: on)\n"
    "                igd-passthru=on|off controls IGD GFX passthrough support (default=off)\n"
    "                aes-key-wrap=on|off controls support for AES key wrapping (default=on)\n"
    "                dea-key-wrap=on|off controls support for DEA key wrapping (default=on)\n"
    "                suppress-vmdesc=on|off disables self-describing migration (default=off)\n"
    "                nvdimm=on|off controls NVDIMM support (default=off)\n"
    "                enforce-config-section=on|off enforce configuration section migration (default=off)\n"
    "                memory-encryption=@var{} memory encryption object to use (default=none)\n",
    QEMU_ARCH_ALL)
STEXI
@item -machine [type=]@var{name}[,prop=@var{value}[,...]]
@findex -machine
Select the emulated machine by @var{name}. Use @code{-machine help} to list
available machines.

For architectures which aim to support live migration compatibility
across releases, each release will introduce a new versioned machine
type. For example, the 2.8.0 release introduced machine types
``pc-i440fx-2.8'' and ``pc-q35-2.8'' for the x86_64/i686 architectures.

To allow live migration of guests from QEMU version 2.8.0, to QEMU
version 2.9.0, the 2.9.0 version must support the ``pc-i440fx-2.8''
and ``pc-q35-2.8'' machines too. To allow users live migrating VMs
to skip multiple intermediate releases when upgrading, new releases
of QEMU will support machine types from many previous versions.

Supported machine properties are:
@table @option
@item accel=@var{accels1}[:@var{accels2}[:...]]
This is used to enable an accelerator. Depending on the target architecture,
kvm, xen, hax, hvf, whpx or tcg can be available. By default, tcg is used. If there is
more than one accelerator specified, the next one is used if the previous one
fails to initialize.
@item kernel_irqchip=on|off
Controls in-kernel irqchip support for the chosen accelerator when available.
@item gfx_passthru=on|off
Enables IGD GFX passthrough support for the chosen machine when available.
@item vmport=on|off|auto
Enables emulation of VMWare IO port, for vmmouse etc. auto says to select the
value based on accel. For accel=xen the default is off otherwise the default
is on.
@item kvm_shadow_mem=size
Defines the size of the KVM shadow MMU.
@item dump-guest-core=on|off
Include guest memory in a core dump. The default is on.
@item mem-merge=on|off
Enables or disables memory merge support. This feature, when supported by
the host, de-duplicates identical memory pages among VMs instances
(enabled by default).
@item aes-key-wrap=on|off
Enables or disables AES key wrapping support on s390-ccw hosts. This feature
controls whether AES wrapping keys will be created to allow
execution of AES cryptographic functions.  The default is on.
@item dea-key-wrap=on|off
Enables or disables DEA key wrapping support on s390-ccw hosts. This feature
controls whether DEA wrapping keys will be created to allow
execution of DEA cryptographic functions.  The default is on.
@item nvdimm=on|off
Enables or disables NVDIMM support. The default is off.
@item enforce-config-section=on|off
If @option{enforce-config-section} is set to @var{on}, force migration
code to send configuration section even if the machine-type sets the
@option{migration.send-configuration} property to @var{off}.
NOTE: this parameter is deprecated. Please use @option{-global}
@option{migration.send-configuration}=@var{on|off} instead.
@item memory-encryption=@var{}
Memory encryption object to use. The default is none.
@end table
ETEXI

HXCOMM Deprecated by -machine
DEF("M", HAS_ARG, QEMU_OPTION_M, "", QEMU_ARCH_ALL)

DEF("cpu", HAS_ARG, QEMU_OPTION_cpu,
    "-cpu cpu        select CPU ('-cpu help' for list)\n", QEMU_ARCH_ALL)
STEXI
@item -cpu @var{model}
@findex -cpu
Select CPU model (@code{-cpu help} for list and additional feature selection)
ETEXI

DEF("accel", HAS_ARG, QEMU_OPTION_accel,
    "-accel [accel=]accelerator[,thread=single|multi]\n"
    "                select accelerator (kvm, xen, hax, hvf, whpx or tcg; use 'help' for a list)\n"
    "                thread=single|multi (enable multi-threaded TCG)\n", QEMU_ARCH_ALL)
STEXI
@item -accel @var{name}[,prop=@var{value}[,...]]
@findex -accel
This is used to enable an accelerator. Depending on the target architecture,
kvm, xen, hax, hvf, whpx or tcg can be available. By default, tcg is used. If there is
more than one accelerator specified, the next one is used if the previous one
fails to initialize.
@table @option
@item thread=single|multi
Controls number of TCG threads. When the TCG is multi-threaded there will be one
thread per vCPU therefor taking advantage of additional host cores. The default
is to enable multi-threading where both the back-end and front-ends support it and
no incompatible TCG features have been enabled (e.g. icount/replay).
@end table
ETEXI

DEF("smp", HAS_ARG, QEMU_OPTION_smp,
    "-smp [cpus=]n[,maxcpus=cpus][,cores=cores][,threads=threads][,dies=dies][,sockets=sockets]\n"
    "                set the number of CPUs to 'n' [default=1]\n"
    "                maxcpus= maximum number of total cpus, including\n"
    "                offline CPUs for hotplug, etc\n"
    "                cores= number of CPU cores on one socket (for PC, it's on one die)\n"
    "                threads= number of threads on one CPU core\n"
    "                dies= number of CPU dies on one socket (for PC only)\n"
    "                sockets= number of discrete sockets in the system\n",
        QEMU_ARCH_ALL)
STEXI
@item -smp [cpus=]@var{n}[,cores=@var{cores}][,threads=@var{threads}][,dies=dies][,sockets=@var{sockets}][,maxcpus=@var{maxcpus}]
@findex -smp
Simulate an SMP system with @var{n} CPUs. On the PC target, up to 255
CPUs are supported. On Sparc32 target, Linux limits the number of usable CPUs
to 4.
For the PC target, the number of @var{cores} per die, the number of @var{threads}
per cores, the number of @var{dies} per packages and the total number of
@var{sockets} can be specified. Missing values will be computed.
If any on the three values is given, the total number of CPUs @var{n} can be omitted.
@var{maxcpus} specifies the maximum number of hotpluggable CPUs.
ETEXI

DEF("vcpu", HAS_ARG, QEMU_OPTION_vcpu,
    "-vcpu [vcpunum=]n[,affinity=affinity]\n"
    "-vcpu [vcpunum=]n[,affinity=affinity]\n", QEMU_ARCH_ALL)
STEXI
@item -vcpu [vcpunum=]@var{n}[,affinity=@var{affinity}]
@itemx -vcpu [vcpunum=]@var{n}[,affinity=@var{affinity}]
@findex -vcpu
VCPU Affinity. If specified, specify for all the CPUs.
ETEXI

DEF("numa", HAS_ARG, QEMU_OPTION_numa,
    "-numa node[,mem=size][,cpus=firstcpu[-lastcpu]][,nodeid=node]\n"
    "-numa node[,memdev=id][,cpus=firstcpu[-lastcpu]][,nodeid=node]\n"
    "-numa dist,src=source,dst=destination,val=distance\n"
    "-numa cpu,node-id=node[,socket-id=x][,core-id=y][,thread-id=z]\n",
    QEMU_ARCH_ALL)
STEXI
@item -numa node[,mem=@var{size}][,cpus=@var{firstcpu}[-@var{lastcpu}]][,nodeid=@var{node}]
@itemx -numa node[,memdev=@var{id}][,cpus=@var{firstcpu}[-@var{lastcpu}]][,nodeid=@var{node}]
@itemx -numa dist,src=@var{source},dst=@var{destination},val=@var{distance}
@itemx -numa cpu,node-id=@var{node}[,socket-id=@var{x}][,core-id=@var{y}][,thread-id=@var{z}]
@findex -numa
Define a NUMA node and assign RAM and VCPUs to it.
Set the NUMA distance from a source node to a destination node.

Legacy VCPU assignment uses @samp{cpus} option where
@var{firstcpu} and @var{lastcpu} are CPU indexes. Each
@samp{cpus} option represent a contiguous range of CPU indexes
(or a single VCPU if @var{lastcpu} is omitted). A non-contiguous
set of VCPUs can be represented by providing multiple @samp{cpus}
options. If @samp{cpus} is omitted on all nodes, VCPUs are automatically
split between them.

For example, the following option assigns VCPUs 0, 1, 2 and 5 to
a NUMA node:
@example
-numa node,cpus=0-2,cpus=5
@end example

@samp{cpu} option is a new alternative to @samp{cpus} option
which uses @samp{socket-id|core-id|thread-id} properties to assign
CPU objects to a @var{node} using topology layout properties of CPU.
The set of properties is machine specific, and depends on used
machine type/@samp{smp} options. It could be queried with
@samp{hotpluggable-cpus} monitor command.
@samp{node-id} property specifies @var{node} to which CPU object
will be assigned, it's required for @var{node} to be declared
with @samp{node} option before it's used with @samp{cpu} option.

For example:
@example
-M pc \
-smp 1,sockets=2,maxcpus=2 \
-numa node,nodeid=0 -numa node,nodeid=1 \
-numa cpu,node-id=0,socket-id=0 -numa cpu,node-id=1,socket-id=1
@end example

@samp{mem} assigns a given RAM amount to a node. @samp{memdev}
assigns RAM from a given memory backend device to a node. If
@samp{mem} and @samp{memdev} are omitted in all nodes, RAM is
split equally between them.

@samp{mem} and @samp{memdev} are mutually exclusive. Furthermore,
if one node uses @samp{memdev}, all of them have to use it.

@var{source} and @var{destination} are NUMA node IDs.
@var{distance} is the NUMA distance from @var{source} to @var{destination}.
The distance from a node to itself is always 10. If any pair of nodes is
given a distance, then all pairs must be given distances. Although, when
distances are only given in one direction for each pair of nodes, then
the distances in the opposite directions are assumed to be the same. If,
however, an asymmetrical pair of distances is given for even one node
pair, then all node pairs must be provided distance values for both
directions, even when they are symmetrical. When a node is unreachable
from another node, set the pair's distance to 255.

Note that the -@option{numa} option doesn't allocate any of the
specified resources, it just assigns existing resources to NUMA
nodes. This means that one still has to use the @option{-m},
@option{-smp} options to allocate RAM and VCPUs respectively.

ETEXI

DEF("add-fd", HAS_ARG, QEMU_OPTION_add_fd,
    "-add-fd fd=fd,set=set[,opaque=opaque]\n"
    "                Add 'fd' to fd 'set'\n", QEMU_ARCH_ALL)
STEXI
@item -add-fd fd=@var{fd},set=@var{set}[,opaque=@var{opaque}]
@findex -add-fd

Add a file descriptor to an fd set.  Valid options are:

@table @option
@item fd=@var{fd}
This option defines the file descriptor of which a duplicate is added to fd set.
The file descriptor cannot be stdin, stdout, or stderr.
@item set=@var{set}
This option defines the ID of the fd set to add the file descriptor to.
@item opaque=@var{opaque}
This option defines a free-form string that can be used to describe @var{fd}.
@end table

You can open an image using pre-opened file descriptors from an fd set:
@example
@value{qemu_system} \
 -add-fd fd=3,set=2,opaque="rdwr:/path/to/file" \
 -add-fd fd=4,set=2,opaque="rdonly:/path/to/file" \
 -drive file=/dev/fdset/2,index=0,media=disk
@end example
ETEXI

DEF("set", HAS_ARG, QEMU_OPTION_set,
    "-set group.id.arg=value\n"
    "                set <arg> parameter for item <id> of type <group>\n"
    "                i.e. -set drive.$id.file=/path/to/image\n", QEMU_ARCH_ALL)
STEXI
@item -set @var{group}.@var{id}.@var{arg}=@var{value}
@findex -set
Set parameter @var{arg} for item @var{id} of type @var{group}
ETEXI

DEF("global", HAS_ARG, QEMU_OPTION_global,
    "-global driver.property=value\n"
    "-global driver=driver,property=property,value=value\n"
    "                set a global default for a driver property\n",
    QEMU_ARCH_ALL)
STEXI
@item -global @var{driver}.@var{prop}=@var{value}
@itemx -global driver=@var{driver},property=@var{property},value=@var{value}
@findex -global
Set default value of @var{driver}'s property @var{prop} to @var{value}, e.g.:

@example
@value{qemu_system_x86} -global ide-hd.physical_block_size=4096 disk-image.img
@end example

In particular, you can use this to set driver properties for devices which are
created automatically by the machine model. To create a device which is not
created automatically and set properties on it, use -@option{device}.

-global @var{driver}.@var{prop}=@var{value} is shorthand for -global
driver=@var{driver},property=@var{prop},value=@var{value}.  The
longhand syntax works even when @var{driver} contains a dot.
ETEXI

DEF("boot", HAS_ARG, QEMU_OPTION_boot,
    "-boot [order=drives][,once=drives][,menu=on|off]\n"
    "      [,splash=sp_name][,splash-time=sp_time][,reboot-timeout=rb_time][,strict=on|off]\n"
    "                'drives': floppy (a), hard disk (c), CD-ROM (d), network (n)\n"
    "                'sp_name': the file's name that would be passed to bios as logo picture, if menu=on\n"
    "                'sp_time': the period that splash picture last if menu=on, unit is ms\n"
    "                'rb_timeout': the timeout before guest reboot when boot failed, unit is ms\n",
    QEMU_ARCH_ALL)
STEXI
@item -boot [order=@var{drives}][,once=@var{drives}][,menu=on|off][,splash=@var{sp_name}][,splash-time=@var{sp_time}][,reboot-timeout=@var{rb_timeout}][,strict=on|off]
@findex -boot
Specify boot order @var{drives} as a string of drive letters. Valid
drive letters depend on the target architecture. The x86 PC uses: a, b
(floppy 1 and 2), c (first hard disk), d (first CD-ROM), n-p (Etherboot
from network adapter 1-4), hard disk boot is the default. To apply a
particular boot order only on the first startup, specify it via
@option{once}. Note that the @option{order} or @option{once} parameter
should not be used together with the @option{bootindex} property of
devices, since the firmware implementations normally do not support both
at the same time.

Interactive boot menus/prompts can be enabled via @option{menu=on} as far
as firmware/BIOS supports them. The default is non-interactive boot.

A splash picture could be passed to bios, enabling user to show it as logo,
when option splash=@var{sp_name} is given and menu=on, If firmware/BIOS
supports them. Currently Seabios for X86 system support it.
limitation: The splash file could be a jpeg file or a BMP file in 24 BPP
format(true color). The resolution should be supported by the SVGA mode, so
the recommended is 320x240, 640x480, 800x640.

A timeout could be passed to bios, guest will pause for @var{rb_timeout} ms
when boot failed, then reboot. If @var{rb_timeout} is '-1', guest will not
reboot, qemu passes '-1' to bios by default. Currently Seabios for X86
system support it.

Do strict boot via @option{strict=on} as far as firmware/BIOS
supports it. This only effects when boot priority is changed by
bootindex options. The default is non-strict boot.

@example
# try to boot from network first, then from hard disk
@value{qemu_system_x86} -boot order=nc
# boot from CD-ROM first, switch back to default order after reboot
@value{qemu_system_x86} -boot once=d
# boot with a splash picture for 5 seconds.
@value{qemu_system_x86} -boot menu=on,splash=/root/boot.bmp,splash-time=5000
@end example

Note: The legacy format '-boot @var{drives}' is still supported but its
use is discouraged as it may be removed from future versions.
ETEXI

DEF("m", HAS_ARG, QEMU_OPTION_m,
    "-m [size=]megs[,slots=n,maxmem=size]\n"
    "                configure guest RAM\n"
    "                size: initial amount of guest memory\n"
    "                slots: number of hotplug slots (default: none)\n"
    "                maxmem: maximum amount of guest memory (default: none)\n"
    "NOTE: Some architectures might enforce a specific granularity\n",
    QEMU_ARCH_ALL)
STEXI
@item -m [size=]@var{megs}[,slots=n,maxmem=size]
@findex -m
Sets guest startup RAM size to @var{megs} megabytes. Default is 128 MiB.
Optionally, a suffix of ``M'' or ``G'' can be used to signify a value in
megabytes or gigabytes respectively. Optional pair @var{slots}, @var{maxmem}
could be used to set amount of hotpluggable memory slots and maximum amount of
memory. Note that @var{maxmem} must be aligned to the page size.

For example, the following command-line sets the guest startup RAM size to
1GB, creates 3 slots to hotplug additional memory and sets the maximum
memory the guest can reach to 4GB:

@example
@value{qemu_system} -m 1G,slots=3,maxmem=4G
@end example

If @var{slots} and @var{maxmem} are not specified, memory hotplug won't
be enabled and the guest startup RAM will never increase.
ETEXI

DEF("mem-path", HAS_ARG, QEMU_OPTION_mempath,
    "-mem-path FILE  provide backing storage for guest RAM\n", QEMU_ARCH_ALL)
STEXI
@item -mem-path @var{path}
@findex -mem-path
Allocate guest RAM from a temporarily created file in @var{path}.
ETEXI

DEF("mem-prealloc", 0, QEMU_OPTION_mem_prealloc,
    "-mem-prealloc   preallocate guest memory (use with -mem-path)\n",
    QEMU_ARCH_ALL)
STEXI
@item -mem-prealloc
@findex -mem-prealloc
Preallocate memory when using -mem-path.
ETEXI

DEF("k", HAS_ARG, QEMU_OPTION_k,
    "-k language     use keyboard layout (for example 'fr' for French)\n",
    QEMU_ARCH_ALL)
STEXI
@item -k @var{language}
@findex -k
Use keyboard layout @var{language} (for example @code{fr} for
French). This option is only needed where it is not easy to get raw PC
keycodes (e.g. on Macs, with some X11 servers or with a VNC or curses
display). You don't normally need to use it on PC/Linux or PC/Windows
hosts.

The available layouts are:
@example
ar  de-ch  es  fo     fr-ca  hu  ja  mk     no  pt-br  sv
da  en-gb  et  fr     fr-ch  is  lt  nl     pl  ru     th
de  en-us  fi  fr-be  hr     it  lv  nl-be  pt  sl     tr
@end example

The default is @code{en-us}.
ETEXI


HXCOMM Deprecated by -audiodev
DEF("audio-help", 0, QEMU_OPTION_audio_help,
    "-audio-help     show -audiodev equivalent of the currently specified audio settings\n",
    QEMU_ARCH_ALL)
STEXI
@item -audio-help
@findex -audio-help
Will show the -audiodev equivalent of the currently specified
(deprecated) environment variables.
ETEXI

DEF("audiodev", HAS_ARG, QEMU_OPTION_audiodev,
    "-audiodev [driver=]driver,id=id[,prop[=value][,...]]\n"
    "                specifies the audio backend to use\n"
    "                id= identifier of the backend\n"
    "                timer-period= timer period in microseconds\n"
    "                in|out.mixing-engine= use mixing engine to mix streams inside QEMU\n"
    "                in|out.fixed-settings= use fixed settings for host audio\n"
    "                in|out.frequency= frequency to use with fixed settings\n"
    "                in|out.channels= number of channels to use with fixed settings\n"
    "                in|out.format= sample format to use with fixed settings\n"
    "                valid values: s8, s16, s32, u8, u16, u32\n"
    "                in|out.voices= number of voices to use\n"
    "                in|out.buffer-length= length of buffer in microseconds\n"
    "-audiodev none,id=id,[,prop[=value][,...]]\n"
    "                dummy driver that discards all output\n"
#ifdef CONFIG_AUDIO_ALSA
    "-audiodev alsa,id=id[,prop[=value][,...]]\n"
    "                in|out.dev= name of the audio device to use\n"
    "                in|out.period-length= length of period in microseconds\n"
    "                in|out.try-poll= attempt to use poll mode\n"
    "                threshold= threshold (in microseconds) when playback starts\n"
#endif
#ifdef CONFIG_AUDIO_COREAUDIO
    "-audiodev coreaudio,id=id[,prop[=value][,...]]\n"
    "                in|out.buffer-count= number of buffers\n"
#endif
#ifdef CONFIG_AUDIO_DSOUND
    "-audiodev dsound,id=id[,prop[=value][,...]]\n"
    "                latency= add extra latency to playback in microseconds\n"
#endif
#ifdef CONFIG_AUDIO_OSS
    "-audiodev oss,id=id[,prop[=value][,...]]\n"
    "                in|out.dev= path of the audio device to use\n"
    "                in|out.buffer-count= number of buffers\n"
    "                in|out.try-poll= attempt to use poll mode\n"
    "                try-mmap= try using memory mapped access\n"
    "                exclusive= open device in exclusive mode\n"
    "                dsp-policy= set timing policy (0..10), -1 to use fragment mode\n"
#endif
#ifdef CONFIG_AUDIO_PA
    "-audiodev pa,id=id[,prop[=value][,...]]\n"
    "                server= PulseAudio server address\n"
    "                in|out.name= source/sink device name\n"
    "                in|out.latency= desired latency in microseconds\n"
#endif
#ifdef CONFIG_AUDIO_SDL
    "-audiodev sdl,id=id[,prop[=value][,...]]\n"
#endif
#ifdef CONFIG_SPICE
    "-audiodev spice,id=id[,prop[=value][,...]]\n"
#endif
    "-audiodev wav,id=id[,prop[=value][,...]]\n"
    "                path= path of wav file to record\n",
    QEMU_ARCH_ALL)
STEXI
@item -audiodev [driver=]@var{driver},id=@var{id}[,@var{prop}[=@var{value}][,...]]
@findex -audiodev
Adds a new audio backend @var{driver} identified by @var{id}.  There are
global and driver specific properties.  Some values can be set
differently for input and output, they're marked with @code{in|out.}.
You can set the input's property with @code{in.@var{prop}} and the
output's property with @code{out.@var{prop}}. For example:
@example
-audiodev alsa,id=example,in.frequency=44110,out.frequency=8000
-audiodev alsa,id=example,out.channels=1 # leaves in.channels unspecified
@end example

NOTE: parameter validation is known to be incomplete, in many cases
specifying an invalid option causes QEMU to print an error message and
continue emulation without sound.

Valid global options are:

@table @option
@item id=@var{identifier}
Identifies the audio backend.

@item timer-period=@var{period}
Sets the timer @var{period} used by the audio subsystem in microseconds.
Default is 10000 (10 ms).

@item in|out.mixing-engine=on|off
Use QEMU's mixing engine to mix all streams inside QEMU and convert
audio formats when not supported by the backend.  When off,
@var{fixed-settings} must be off too.  Note that disabling this option
means that the selected backend must support multiple streams and the
audio formats used by the virtual cards, otherwise you'll get no sound.
It's not recommended to disable this option unless you want to use 5.1
or 7.1 audio, as mixing engine only supports mono and stereo audio.
Default is on.

@item in|out.fixed-settings=on|off
Use fixed settings for host audio.  When off, it will change based on
how the guest opens the sound card.  In this case you must not specify
@var{frequency}, @var{channels} or @var{format}.  Default is on.

@item in|out.frequency=@var{frequency}
Specify the @var{frequency} to use when using @var{fixed-settings}.
Default is 44100Hz.

@item in|out.channels=@var{channels}
Specify the number of @var{channels} to use when using
@var{fixed-settings}. Default is 2 (stereo).

@item in|out.format=@var{format}
Specify the sample @var{format} to use when using @var{fixed-settings}.
Valid values are: @code{s8}, @code{s16}, @code{s32}, @code{u8},
@code{u16}, @code{u32}. Default is @code{s16}.

@item in|out.voices=@var{voices}
Specify the number of @var{voices} to use.  Default is 1.

@item in|out.buffer-length=@var{usecs}
Sets the size of the buffer in microseconds.

@end table

@item -audiodev none,id=@var{id}[,@var{prop}[=@var{value}][,...]]
Creates a dummy backend that discards all outputs.  This backend has no
backend specific properties.

@item -audiodev alsa,id=@var{id}[,@var{prop}[=@var{value}][,...]]
Creates backend using the ALSA.  This backend is only available on
Linux.

ALSA specific options are:

@table @option

@item in|out.dev=@var{device}
Specify the ALSA @var{device} to use for input and/or output.  Default
is @code{default}.

@item in|out.period-length=@var{usecs}
Sets the period length in microseconds.

@item in|out.try-poll=on|off
Attempt to use poll mode with the device.  Default is on.

@item threshold=@var{threshold}
Threshold (in microseconds) when playback starts.  Default is 0.

@end table

@item -audiodev coreaudio,id=@var{id}[,@var{prop}[=@var{value}][,...]]
Creates a backend using Apple's Core Audio.  This backend is only
available on Mac OS and only supports playback.

Core Audio specific options are:

@table @option

@item in|out.buffer-count=@var{count}
Sets the @var{count} of the buffers.

@end table

@item -audiodev dsound,id=@var{id}[,@var{prop}[=@var{value}][,...]]
Creates a backend using Microsoft's DirectSound.  This backend is only
available on Windows and only supports playback.

DirectSound specific options are:

@table @option

@item latency=@var{usecs}
Add extra @var{usecs} microseconds latency to playback.  Default is
10000 (10 ms).

@end table

@item -audiodev oss,id=@var{id}[,@var{prop}[=@var{value}][,...]]
Creates a backend using OSS.  This backend is available on most
Unix-like systems.

OSS specific options are:

@table @option

@item in|out.dev=@var{device}
Specify the file name of the OSS @var{device} to use.  Default is
@code{/dev/dsp}.

@item in|out.buffer-count=@var{count}
Sets the @var{count} of the buffers.

@item in|out.try-poll=on|of
Attempt to use poll mode with the device.  Default is on.

@item try-mmap=on|off
Try using memory mapped device access.  Default is off.

@item exclusive=on|off
Open the device in exclusive mode (vmix won't work in this case).
Default is off.

@item dsp-policy=@var{policy}
Sets the timing policy (between 0 and 10, where smaller number means
smaller latency but higher CPU usage).  Use -1 to use buffer sizes
specified by @code{buffer} and @code{buffer-count}.  This option is
ignored if you do not have OSS 4. Default is 5.

@end table

@item -audiodev pa,id=@var{id}[,@var{prop}[=@var{value}][,...]]
Creates a backend using PulseAudio.  This backend is available on most
systems.

PulseAudio specific options are:

@table @option

@item server=@var{server}
Sets the PulseAudio @var{server} to connect to.

@item in|out.name=@var{sink}
Use the specified source/sink for recording/playback.

@item in|out.latency=@var{usecs}
Desired latency in microseconds.  The PulseAudio server will try to honor this
value but actual latencies may be lower or higher.

@end table

@item -audiodev sdl,id=@var{id}[,@var{prop}[=@var{value}][,...]]
Creates a backend using SDL.  This backend is available on most systems,
but you should use your platform's native backend if possible.  This
backend has no backend specific properties.

@item -audiodev spice,id=@var{id}[,@var{prop}[=@var{value}][,...]]
Creates a backend that sends audio through SPICE.  This backend requires
@code{-spice} and automatically selected in that case, so usually you
can ignore this option.  This backend has no backend specific
properties.

@item -audiodev wav,id=@var{id}[,@var{prop}[=@var{value}][,...]]
Creates a backend that writes audio to a WAV file.

Backend specific options are:

@table @option

@item path=@var{path}
Write recorded audio into the specified file.  Default is
@code{qemu.wav}.

@end table
ETEXI

DEF("soundhw", HAS_ARG, QEMU_OPTION_soundhw,
    "-soundhw c1,... enable audio support\n"
    "                and only specified sound cards (comma separated list)\n"
    "                use '-soundhw help' to get the list of supported cards\n"
    "                use '-soundhw all' to enable all of them\n", QEMU_ARCH_ALL)
STEXI
@item -soundhw @var{card1}[,@var{card2},...] or -soundhw all
@findex -soundhw
Enable audio and selected sound hardware. Use 'help' to print all
available sound hardware. For example:

@example
@value{qemu_system_x86} -soundhw sb16,adlib disk.img
@value{qemu_system_x86} -soundhw es1370 disk.img
@value{qemu_system_x86} -soundhw ac97 disk.img
@value{qemu_system_x86} -soundhw hda disk.img
@value{qemu_system_x86} -soundhw all disk.img
@value{qemu_system_x86} -soundhw help
@end example

Note that Linux's i810_audio OSS kernel (for AC97) module might
require manually specifying clocking.

@example
modprobe i810_audio clocking=48000
@end example
ETEXI

DEF("device", HAS_ARG, QEMU_OPTION_device,
    "-device driver[,prop[=value][,...]]\n"
    "                add device (based on driver)\n"
    "                prop=value,... sets driver properties\n"
    "                use '-device help' to print all possible drivers\n"
    "                use '-device driver,help' to print all possible properties\n",
    QEMU_ARCH_ALL)
STEXI
@item -device @var{driver}[,@var{prop}[=@var{value}][,...]]
@findex -device
Add device @var{driver}.  @var{prop}=@var{value} sets driver
properties.  Valid properties depend on the driver.  To get help on
possible drivers and properties, use @code{-device help} and
@code{-device @var{driver},help}.

Some drivers are:
@item -device ipmi-bmc-sim,id=@var{id}[,slave_addr=@var{val}][,sdrfile=@var{file}][,furareasize=@var{val}][,furdatafile=@var{file}][,guid=@var{uuid}]

Add an IPMI BMC.  This is a simulation of a hardware management
interface processor that normally sits on a system.  It provides
a watchdog and the ability to reset and power control the system.
You need to connect this to an IPMI interface to make it useful

The IPMI slave address to use for the BMC.  The default is 0x20.
This address is the BMC's address on the I2C network of management
controllers.  If you don't know what this means, it is safe to ignore
it.

@table @option
@item id=@var{id}
The BMC id for interfaces to use this device.
@item slave_addr=@var{val}
Define slave address to use for the BMC.  The default is 0x20.
@item sdrfile=@var{file}
file containing raw Sensor Data Records (SDR) data. The default is none.
@item fruareasize=@var{val}
size of a Field Replaceable Unit (FRU) area.  The default is 1024.
@item frudatafile=@var{file}
file containing raw Field Replaceable Unit (FRU) inventory data. The default is none.
@item guid=@var{uuid}
value for the GUID for the BMC, in standard UUID format.  If this is set,
get "Get GUID" command to the BMC will return it.  Otherwise "Get GUID"
will return an error.
@end table

@item -device ipmi-bmc-extern,id=@var{id},chardev=@var{id}[,slave_addr=@var{val}]

Add a connection to an external IPMI BMC simulator.  Instead of
locally emulating the BMC like the above item, instead connect
to an external entity that provides the IPMI services.

A connection is made to an external BMC simulator.  If you do this, it
is strongly recommended that you use the "reconnect=" chardev option
to reconnect to the simulator if the connection is lost.  Note that if
this is not used carefully, it can be a security issue, as the
interface has the ability to send resets, NMIs, and power off the VM.
It's best if QEMU makes a connection to an external simulator running
on a secure port on localhost, so neither the simulator nor QEMU is
exposed to any outside network.

See the "lanserv/README.vm" file in the OpenIPMI library for more
details on the external interface.

@item -device isa-ipmi-kcs,bmc=@var{id}[,ioport=@var{val}][,irq=@var{val}]

Add a KCS IPMI interafce on the ISA bus.  This also adds a
corresponding ACPI and SMBIOS entries, if appropriate.

@table @option
@item bmc=@var{id}
The BMC to connect to, one of ipmi-bmc-sim or ipmi-bmc-extern above.
@item ioport=@var{val}
Define the I/O address of the interface.  The default is 0xca0 for KCS.
@item irq=@var{val}
Define the interrupt to use.  The default is 5.  To disable interrupts,
set this to 0.
@end table

@item -device isa-ipmi-bt,bmc=@var{id}[,ioport=@var{val}][,irq=@var{val}]

Like the KCS interface, but defines a BT interface.  The default port is
0xe4 and the default interrupt is 5.

ETEXI

DEF("name", HAS_ARG, QEMU_OPTION_name,
    "-name string1[,process=string2][,debug-threads=on|off]\n"
    "                set the name of the guest\n"
    "                string1 sets the window title and string2 the process name\n"
    "                When debug-threads is enabled, individual threads are given a separate name\n"
    "                NOTE: The thread names are for debugging and not a stable API.\n",
    QEMU_ARCH_ALL)
STEXI
@item -name @var{name}
@findex -name
Sets the @var{name} of the guest.
This name will be displayed in the SDL window caption.
The @var{name} will also be used for the VNC server.
Also optionally set the top visible process name in Linux.
Naming of individual threads can also be enabled on Linux to aid debugging.
ETEXI

DEF("uuid", HAS_ARG, QEMU_OPTION_uuid,
    "-uuid %08x-%04x-%04x-%04x-%012x\n"
    "                specify machine UUID\n", QEMU_ARCH_ALL)
STEXI
@item -uuid @var{uuid}
@findex -uuid
Set system UUID.
ETEXI

STEXI
@end table
ETEXI
DEFHEADING()

DEFHEADING(Block device options:)
STEXI
@table @option
ETEXI

DEF("fda", HAS_ARG, QEMU_OPTION_fda,
    "-fda/-fdb file  use 'file' as floppy disk 0/1 image\n", QEMU_ARCH_ALL)
DEF("fdb", HAS_ARG, QEMU_OPTION_fdb, "", QEMU_ARCH_ALL)
STEXI
@item -fda @var{file}
@itemx -fdb @var{file}
@findex -fda
@findex -fdb
Use @var{file} as floppy disk 0/1 image (@pxref{disk_images}).
ETEXI

DEF("hda", HAS_ARG, QEMU_OPTION_hda,
    "-hda/-hdb file  use 'file' as IDE hard disk 0/1 image\n", QEMU_ARCH_ALL)
DEF("hdb", HAS_ARG, QEMU_OPTION_hdb, "", QEMU_ARCH_ALL)
DEF("hdc", HAS_ARG, QEMU_OPTION_hdc,
    "-hdc/-hdd file  use 'file' as IDE hard disk 2/3 image\n", QEMU_ARCH_ALL)
DEF("hdd", HAS_ARG, QEMU_OPTION_hdd, "", QEMU_ARCH_ALL)
STEXI
@item -hda @var{file}
@itemx -hdb @var{file}
@itemx -hdc @var{file}
@itemx -hdd @var{file}
@findex -hda
@findex -hdb
@findex -hdc
@findex -hdd
Use @var{file} as hard disk 0, 1, 2 or 3 image (@pxref{disk_images}).
ETEXI

DEF("cdrom", HAS_ARG, QEMU_OPTION_cdrom,
    "-cdrom file     use 'file' as IDE cdrom image (cdrom is ide1 master)\n",
    QEMU_ARCH_ALL)
STEXI
@item -cdrom @var{file}
@findex -cdrom
Use @var{file} as CD-ROM image (you cannot use @option{-hdc} and
@option{-cdrom} at the same time). You can use the host CD-ROM by
using @file{/dev/cdrom} as filename (@pxref{host_drives}).
ETEXI

DEF("blockdev", HAS_ARG, QEMU_OPTION_blockdev,
    "-blockdev [driver=]driver[,node-name=N][,discard=ignore|unmap]\n"
    "          [,cache.direct=on|off][,cache.no-flush=on|off]\n"
    "          [,read-only=on|off][,auto-read-only=on|off]\n"
    "          [,force-share=on|off][,detect-zeroes=on|off|unmap]\n"
    "          [,driver specific parameters...]\n"
    "                configure a block backend\n", QEMU_ARCH_ALL)
STEXI
@item -blockdev @var{option}[,@var{option}[,@var{option}[,...]]]
@findex -blockdev

Define a new block driver node. Some of the options apply to all block drivers,
other options are only accepted for a specific block driver. See below for a
list of generic options and options for the most common block drivers.

Options that expect a reference to another node (e.g. @code{file}) can be
given in two ways. Either you specify the node name of an already existing node
(file=@var{node-name}), or you define a new node inline, adding options
for the referenced node after a dot (file.filename=@var{path},file.aio=native).

A block driver node created with @option{-blockdev} can be used for a guest
device by specifying its node name for the @code{drive} property in a
@option{-device} argument that defines a block device.

@table @option
@item Valid options for any block driver node:

@table @code
@item driver
Specifies the block driver to use for the given node.
@item node-name
This defines the name of the block driver node by which it will be referenced
later. The name must be unique, i.e. it must not match the name of a different
block driver node, or (if you use @option{-drive} as well) the ID of a drive.

If no node name is specified, it is automatically generated. The generated node
name is not intended to be predictable and changes between QEMU invocations.
For the top level, an explicit node name must be specified.
@item read-only
Open the node read-only. Guest write attempts will fail.

Note that some block drivers support only read-only access, either generally or
in certain configurations. In this case, the default value
@option{read-only=off} does not work and the option must be specified
explicitly.
@item auto-read-only
If @option{auto-read-only=on} is set, QEMU may fall back to read-only usage
even when @option{read-only=off} is requested, or even switch between modes as
needed, e.g. depending on whether the image file is writable or whether a
writing user is attached to the node.
@item force-share
Override the image locking system of QEMU by forcing the node to utilize
weaker shared access for permissions where it would normally request exclusive
access.  When there is the potential for multiple instances to have the same
file open (whether this invocation of QEMU is the first or the second
instance), both instances must permit shared access for the second instance to
succeed at opening the file.

Enabling @option{force-share=on} requires @option{read-only=on}.
@item cache.direct
The host page cache can be avoided with @option{cache.direct=on}. This will
attempt to do disk IO directly to the guest's memory. QEMU may still perform an
internal copy of the data.
@item cache.no-flush
In case you don't care about data integrity over host failures, you can use
@option{cache.no-flush=on}. This option tells QEMU that it never needs to write
any data to the disk but can instead keep things in cache. If anything goes
wrong, like your host losing power, the disk storage getting disconnected
accidentally, etc. your image will most probably be rendered unusable.
@item discard=@var{discard}
@var{discard} is one of "ignore" (or "off") or "unmap" (or "on") and controls
whether @code{discard} (also known as @code{trim} or @code{unmap}) requests are
ignored or passed to the filesystem. Some machine types may not support
discard requests.
@item detect-zeroes=@var{detect-zeroes}
@var{detect-zeroes} is "off", "on" or "unmap" and enables the automatic
conversion of plain zero writes by the OS to driver specific optimized
zero write commands. You may even choose "unmap" if @var{discard} is set
to "unmap" to allow a zero write to be converted to an @code{unmap} operation.
@end table

@item Driver-specific options for @code{file}

This is the protocol-level block driver for accessing regular files.

@table @code
@item filename
The path to the image file in the local filesystem
@item aio
Specifies the AIO backend (threads/native, default: threads)
@item locking
Specifies whether the image file is protected with Linux OFD / POSIX locks. The
default is to use the Linux Open File Descriptor API if available, otherwise no
lock is applied.  (auto/on/off, default: auto)
@end table
Example:
@example
-blockdev driver=file,node-name=disk,filename=disk.img
@end example

@item Driver-specific options for @code{raw}

This is the image format block driver for raw images. It is usually
stacked on top of a protocol level block driver such as @code{file}.

@table @code
@item file
Reference to or definition of the data source block driver node
(e.g. a @code{file} driver node)
@end table
Example 1:
@example
-blockdev driver=file,node-name=disk_file,filename=disk.img
-blockdev driver=raw,node-name=disk,file=disk_file
@end example
Example 2:
@example
-blockdev driver=raw,node-name=disk,file.driver=file,file.filename=disk.img
@end example

@item Driver-specific options for @code{qcow2}

This is the image format block driver for qcow2 images. It is usually
stacked on top of a protocol level block driver such as @code{file}.

@table @code
@item file
Reference to or definition of the data source block driver node
(e.g. a @code{file} driver node)

@item backing
Reference to or definition of the backing file block device (default is taken
from the image file). It is allowed to pass @code{null} here in order to disable
the default backing file.

@item lazy-refcounts
Whether to enable the lazy refcounts feature (on/off; default is taken from the
image file)

@item cache-size
The maximum total size of the L2 table and refcount block caches in bytes
(default: the sum of l2-cache-size and refcount-cache-size)

@item l2-cache-size
The maximum size of the L2 table cache in bytes
(default: if cache-size is not specified - 32M on Linux platforms, and 8M on
non-Linux platforms; otherwise, as large as possible within the cache-size,
while permitting the requested or the minimal refcount cache size)

@item refcount-cache-size
The maximum size of the refcount block cache in bytes
(default: 4 times the cluster size; or if cache-size is specified, the part of
it which is not used for the L2 cache)

@item cache-clean-interval
Clean unused entries in the L2 and refcount caches. The interval is in seconds.
The default value is 600 on supporting platforms, and 0 on other platforms.
Setting it to 0 disables this feature.

@item pass-discard-request
Whether discard requests to the qcow2 device should be forwarded to the data
source (on/off; default: on if discard=unmap is specified, off otherwise)

@item pass-discard-snapshot
Whether discard requests for the data source should be issued when a snapshot
operation (e.g. deleting a snapshot) frees clusters in the qcow2 file (on/off;
default: on)

@item pass-discard-other
Whether discard requests for the data source should be issued on other
occasions where a cluster gets freed (on/off; default: off)

@item overlap-check
Which overlap checks to perform for writes to the image
(none/constant/cached/all; default: cached). For details or finer
granularity control refer to the QAPI documentation of @code{blockdev-add}.
@end table

Example 1:
@example
-blockdev driver=file,node-name=my_file,filename=/tmp/disk.qcow2
-blockdev driver=qcow2,node-name=hda,file=my_file,overlap-check=none,cache-size=16777216
@end example
Example 2:
@example
-blockdev driver=qcow2,node-name=disk,file.driver=http,file.filename=http://example.com/image.qcow2
@end example

@item Driver-specific options for other drivers
Please refer to the QAPI documentation of the @code{blockdev-add} QMP command.

@end table

ETEXI

DEF("drive", HAS_ARG, QEMU_OPTION_drive,
    "-drive [file=file][,if=type][,bus=n][,unit=m][,media=d][,index=i]\n"
    "       [,cache=writethrough|writeback|none|directsync|unsafe][,format=f]\n"
    "       [,snapshot=on|off][,rerror=ignore|stop|report]\n"
    "       [,werror=ignore|stop|report|enospc][,id=name][,aio=threads|native]\n"
    "       [,readonly=on|off][,copy-on-read=on|off]\n"
    "       [,discard=ignore|unmap][,detect-zeroes=on|off|unmap]\n"
    "       [[,bps=b]|[[,bps_rd=r][,bps_wr=w]]]\n"
    "       [[,iops=i]|[[,iops_rd=r][,iops_wr=w]]]\n"
    "       [[,bps_max=bm]|[[,bps_rd_max=rm][,bps_wr_max=wm]]]\n"
    "       [[,iops_max=im]|[[,iops_rd_max=irm][,iops_wr_max=iwm]]]\n"
    "       [[,iops_size=is]]\n"
    "       [[,group=g]]\n"
    "                use 'file' as a drive image\n", QEMU_ARCH_ALL)
STEXI
@item -drive @var{option}[,@var{option}[,@var{option}[,...]]]
@findex -drive

Define a new drive. This includes creating a block driver node (the backend) as
well as a guest device, and is mostly a shortcut for defining the corresponding
@option{-blockdev} and @option{-device} options.

@option{-drive} accepts all options that are accepted by @option{-blockdev}. In
addition, it knows the following options:

@table @option
@item file=@var{file}
This option defines which disk image (@pxref{disk_images}) to use with
this drive. If the filename contains comma, you must double it
(for instance, "file=my,,file" to use file "my,file").

Special files such as iSCSI devices can be specified using protocol
specific URLs. See the section for "Device URL Syntax" for more information.
@item if=@var{interface}
This option defines on which type on interface the drive is connected.
Available types are: ide, scsi, sd, mtd, floppy, pflash, virtio, none.
@item bus=@var{bus},unit=@var{unit}
These options define where is connected the drive by defining the bus number and
the unit id.
@item index=@var{index}
This option defines where is connected the drive by using an index in the list
of available connectors of a given interface type.
@item media=@var{media}
This option defines the type of the media: disk or cdrom.
@item snapshot=@var{snapshot}
@var{snapshot} is "on" or "off" and controls snapshot mode for the given drive
(see @option{-snapshot}).
@item cache=@var{cache}
@var{cache} is "none", "writeback", "unsafe", "directsync" or "writethrough"
and controls how the host cache is used to access block data. This is a
shortcut that sets the @option{cache.direct} and @option{cache.no-flush}
options (as in @option{-blockdev}), and additionally @option{cache.writeback},
which provides a default for the @option{write-cache} option of block guest
devices (as in @option{-device}). The modes correspond to the following
settings:

@c Our texi2pod.pl script doesn't support @multitable, so fall back to using
@c plain ASCII art (well, UTF-8 art really). This looks okay both in the manpage
@c and the HTML output.
@example
@             │ cache.writeback   cache.direct   cache.no-flush
─────────────┼─────────────────────────────────────────────────
writeback    │ on                off            off
none         │ on                on             off
writethrough │ off               off            off
directsync   │ off               on             off
unsafe       │ on                off            on
@end example

The default mode is @option{cache=writeback}.

@item aio=@var{aio}
@var{aio} is "threads", or "native" and selects between pthread based disk I/O and native Linux AIO.
@item format=@var{format}
Specify which disk @var{format} will be used rather than detecting
the format.  Can be used to specify format=raw to avoid interpreting
an untrusted format header.
@item werror=@var{action},rerror=@var{action}
Specify which @var{action} to take on write and read errors. Valid actions are:
"ignore" (ignore the error and try to continue), "stop" (pause QEMU),
"report" (report the error to the guest), "enospc" (pause QEMU only if the
host disk is full; report the error to the guest otherwise).
The default setting is @option{werror=enospc} and @option{rerror=report}.
@item copy-on-read=@var{copy-on-read}
@var{copy-on-read} is "on" or "off" and enables whether to copy read backing
file sectors into the image file.
@item bps=@var{b},bps_rd=@var{r},bps_wr=@var{w}
Specify bandwidth throttling limits in bytes per second, either for all request
types or for reads or writes only.  Small values can lead to timeouts or hangs
inside the guest.  A safe minimum for disks is 2 MB/s.
@item bps_max=@var{bm},bps_rd_max=@var{rm},bps_wr_max=@var{wm}
Specify bursts in bytes per second, either for all request types or for reads
or writes only.  Bursts allow the guest I/O to spike above the limit
temporarily.
@item iops=@var{i},iops_rd=@var{r},iops_wr=@var{w}
Specify request rate limits in requests per second, either for all request
types or for reads or writes only.
@item iops_max=@var{bm},iops_rd_max=@var{rm},iops_wr_max=@var{wm}
Specify bursts in requests per second, either for all request types or for reads
or writes only.  Bursts allow the guest I/O to spike above the limit
temporarily.
@item iops_size=@var{is}
Let every @var{is} bytes of a request count as a new request for iops
throttling purposes.  Use this option to prevent guests from circumventing iops
limits by sending fewer but larger requests.
@item group=@var{g}
Join a throttling quota group with given name @var{g}.  All drives that are
members of the same group are accounted for together.  Use this option to
prevent guests from circumventing throttling limits by using many small disks
instead of a single larger disk.
@end table

By default, the @option{cache.writeback=on} mode is used. It will report data
writes as completed as soon as the data is present in the host page cache.
This is safe as long as your guest OS makes sure to correctly flush disk caches
where needed. If your guest OS does not handle volatile disk write caches
correctly and your host crashes or loses power, then the guest may experience
data corruption.

For such guests, you should consider using @option{cache.writeback=off}. This
means that the host page cache will be used to read and write data, but write
notification will be sent to the guest only after QEMU has made sure to flush
each write to the disk. Be aware that this has a major impact on performance.

When using the @option{-snapshot} option, unsafe caching is always used.

Copy-on-read avoids accessing the same backing file sectors repeatedly and is
useful when the backing file is over a slow network.  By default copy-on-read
is off.

Instead of @option{-cdrom} you can use:
@example
@value{qemu_system} -drive file=file,index=2,media=cdrom
@end example

Instead of @option{-hda}, @option{-hdb}, @option{-hdc}, @option{-hdd}, you can
use:
@example
@value{qemu_system} -drive file=file,index=0,media=disk
@value{qemu_system} -drive file=file,index=1,media=disk
@value{qemu_system} -drive file=file,index=2,media=disk
@value{qemu_system} -drive file=file,index=3,media=disk
@end example

You can open an image using pre-opened file descriptors from an fd set:
@example
@value{qemu_system} \
 -add-fd fd=3,set=2,opaque="rdwr:/path/to/file" \
 -add-fd fd=4,set=2,opaque="rdonly:/path/to/file" \
 -drive file=/dev/fdset/2,index=0,media=disk
@end example

You can connect a CDROM to the slave of ide0:
@example
@value{qemu_system_x86} -drive file=file,if=ide,index=1,media=cdrom
@end example

If you don't specify the "file=" argument, you define an empty drive:
@example
@value{qemu_system_x86} -drive if=ide,index=1,media=cdrom
@end example

Instead of @option{-fda}, @option{-fdb}, you can use:
@example
@value{qemu_system_x86} -drive file=file,index=0,if=floppy
@value{qemu_system_x86} -drive file=file,index=1,if=floppy
@end example

By default, @var{interface} is "ide" and @var{index} is automatically
incremented:
@example
@value{qemu_system_x86} -drive file=a -drive file=b"
@end example
is interpreted like:
@example
@value{qemu_system_x86} -hda a -hdb b
@end example
ETEXI

DEF("mtdblock", HAS_ARG, QEMU_OPTION_mtdblock,
    "-mtdblock file  use 'file' as on-board Flash memory image\n",
    QEMU_ARCH_ALL)
STEXI
@item -mtdblock @var{file}
@findex -mtdblock
Use @var{file} as on-board Flash memory image.
ETEXI

DEF("sd", HAS_ARG, QEMU_OPTION_sd,
    "-sd file        use 'file' as SecureDigital card image\n", QEMU_ARCH_ALL)
STEXI
@item -sd @var{file}
@findex -sd
Use @var{file} as SecureDigital card image.
ETEXI

DEF("pflash", HAS_ARG, QEMU_OPTION_pflash,
    "-pflash file    use 'file' as a parallel flash image\n", QEMU_ARCH_ALL)
STEXI
@item -pflash @var{file}
@findex -pflash
Use @var{file} as a parallel flash image.
ETEXI

DEF("snapshot", 0, QEMU_OPTION_snapshot,
    "-snapshot       write to temporary files instead of disk image files\n",
    QEMU_ARCH_ALL)
STEXI
@item -snapshot
@findex -snapshot
Write to temporary files instead of disk image files. In this case,
the raw disk image you use is not written back. You can however force
the write back by pressing @key{C-a s} (@pxref{disk_images}).
ETEXI

DEF("fsdev", HAS_ARG, QEMU_OPTION_fsdev,
    "-fsdev local,id=id,path=path,security_model=mapped-xattr|mapped-file|passthrough|none\n"
    " [,writeout=immediate][,readonly][,fmode=fmode][,dmode=dmode]\n"
    " [[,throttling.bps-total=b]|[[,throttling.bps-read=r][,throttling.bps-write=w]]]\n"
    " [[,throttling.iops-total=i]|[[,throttling.iops-read=r][,throttling.iops-write=w]]]\n"
    " [[,throttling.bps-total-max=bm]|[[,throttling.bps-read-max=rm][,throttling.bps-write-max=wm]]]\n"
    " [[,throttling.iops-total-max=im]|[[,throttling.iops-read-max=irm][,throttling.iops-write-max=iwm]]]\n"
    " [[,throttling.iops-size=is]]\n"
    "-fsdev proxy,id=id,socket=socket[,writeout=immediate][,readonly]\n"
    "-fsdev proxy,id=id,sock_fd=sock_fd[,writeout=immediate][,readonly]\n"
    "-fsdev synth,id=id\n",
    QEMU_ARCH_ALL)

STEXI

@item -fsdev local,id=@var{id},path=@var{path},security_model=@var{security_model} [,writeout=@var{writeout}][,readonly][,fmode=@var{fmode}][,dmode=@var{dmode}] [,throttling.@var{option}=@var{value}[,throttling.@var{option}=@var{value}[,...]]]
@itemx -fsdev proxy,id=@var{id},socket=@var{socket}[,writeout=@var{writeout}][,readonly]
@itemx -fsdev proxy,id=@var{id},sock_fd=@var{sock_fd}[,writeout=@var{writeout}][,readonly]
@itemx -fsdev synth,id=@var{id}[,readonly]
@findex -fsdev
Define a new file system device. Valid options are:
@table @option
@item local
Accesses to the filesystem are done by QEMU.
@item proxy
Accesses to the filesystem are done by virtfs-proxy-helper(1).
@item synth
Synthetic filesystem, only used by QTests.
@item id=@var{id}
Specifies identifier for this device.
@item path=@var{path}
Specifies the export path for the file system device. Files under
this path will be available to the 9p client on the guest.
@item security_model=@var{security_model}
Specifies the security model to be used for this export path.
Supported security models are "passthrough", "mapped-xattr", "mapped-file" and "none".
In "passthrough" security model, files are stored using the same
credentials as they are created on the guest. This requires QEMU
to run as root. In "mapped-xattr" security model, some of the file
attributes like uid, gid, mode bits and link target are stored as
file attributes. For "mapped-file" these attributes are stored in the
hidden .virtfs_metadata directory. Directories exported by this security model cannot
interact with other unix tools. "none" security model is same as
passthrough except the sever won't report failures if it fails to
set file attributes like ownership. Security model is mandatory
only for local fsdriver. Other fsdrivers (like proxy) don't take
security model as a parameter.
@item writeout=@var{writeout}
This is an optional argument. The only supported value is "immediate".
This means that host page cache will be used to read and write data but
write notification will be sent to the guest only when the data has been
reported as written by the storage subsystem.
@item readonly
Enables exporting 9p share as a readonly mount for guests. By default
read-write access is given.
@item socket=@var{socket}
Enables proxy filesystem driver to use passed socket file for communicating
with virtfs-proxy-helper(1).
@item sock_fd=@var{sock_fd}
Enables proxy filesystem driver to use passed socket descriptor for
communicating with virtfs-proxy-helper(1). Usually a helper like libvirt
will create socketpair and pass one of the fds as sock_fd.
@item fmode=@var{fmode}
Specifies the default mode for newly created files on the host. Works only
with security models "mapped-xattr" and "mapped-file".
@item dmode=@var{dmode}
Specifies the default mode for newly created directories on the host. Works
only with security models "mapped-xattr" and "mapped-file".
@item throttling.bps-total=@var{b},throttling.bps-read=@var{r},throttling.bps-write=@var{w}
Specify bandwidth throttling limits in bytes per second, either for all request
types or for reads or writes only.
@item throttling.bps-total-max=@var{bm},bps-read-max=@var{rm},bps-write-max=@var{wm}
Specify bursts in bytes per second, either for all request types or for reads
or writes only.  Bursts allow the guest I/O to spike above the limit
temporarily.
@item throttling.iops-total=@var{i},throttling.iops-read=@var{r}, throttling.iops-write=@var{w}
Specify request rate limits in requests per second, either for all request
types or for reads or writes only.
@item throttling.iops-total-max=@var{im},throttling.iops-read-max=@var{irm}, throttling.iops-write-max=@var{iwm}
Specify bursts in requests per second, either for all request types or for reads
or writes only.  Bursts allow the guest I/O to spike above the limit temporarily.
@item throttling.iops-size=@var{is}
Let every @var{is} bytes of a request count as a new request for iops
throttling purposes.
@end table

-fsdev option is used along with -device driver "virtio-9p-...".
@item -device virtio-9p-@var{type},fsdev=@var{id},mount_tag=@var{mount_tag}
Options for virtio-9p-... driver are:
@table @option
@item @var{type}
Specifies the variant to be used. Supported values are "pci", "ccw" or "device",
depending on the machine type.
@item fsdev=@var{id}
Specifies the id value specified along with -fsdev option.
@item mount_tag=@var{mount_tag}
Specifies the tag name to be used by the guest to mount this export point.
@end table

ETEXI

DEF("virtfs", HAS_ARG, QEMU_OPTION_virtfs,
    "-virtfs local,path=path,mount_tag=tag,security_model=mapped-xattr|mapped-file|passthrough|none\n"
    "        [,id=id][,writeout=immediate][,readonly][,fmode=fmode][,dmode=dmode][,multidevs=remap|forbid|warn]\n"
    "-virtfs proxy,mount_tag=tag,socket=socket[,id=id][,writeout=immediate][,readonly]\n"
    "-virtfs proxy,mount_tag=tag,sock_fd=sock_fd[,id=id][,writeout=immediate][,readonly]\n"
    "-virtfs synth,mount_tag=tag[,id=id][,readonly]\n",
    QEMU_ARCH_ALL)

STEXI

@item -virtfs local,path=@var{path},mount_tag=@var{mount_tag} ,security_model=@var{security_model}[,writeout=@var{writeout}][,readonly] [,fmode=@var{fmode}][,dmode=@var{dmode}][,multidevs=@var{multidevs}]
@itemx -virtfs proxy,socket=@var{socket},mount_tag=@var{mount_tag} [,writeout=@var{writeout}][,readonly]
@itemx -virtfs proxy,sock_fd=@var{sock_fd},mount_tag=@var{mount_tag} [,writeout=@var{writeout}][,readonly]
@itemx -virtfs synth,mount_tag=@var{mount_tag}
@findex -virtfs

Define a new filesystem device and expose it to the guest using a virtio-9p-device. The general form of a Virtual File system pass-through options are:
@table @option
@item local
Accesses to the filesystem are done by QEMU.
@item proxy
Accesses to the filesystem are done by virtfs-proxy-helper(1).
@item synth
Synthetic filesystem, only used by QTests.
@item id=@var{id}
Specifies identifier for the filesystem device
@item path=@var{path}
Specifies the export path for the file system device. Files under
this path will be available to the 9p client on the guest.
@item security_model=@var{security_model}
Specifies the security model to be used for this export path.
Supported security models are "passthrough", "mapped-xattr", "mapped-file" and "none".
In "passthrough" security model, files are stored using the same
credentials as they are created on the guest. This requires QEMU
to run as root. In "mapped-xattr" security model, some of the file
attributes like uid, gid, mode bits and link target are stored as
file attributes. For "mapped-file" these attributes are stored in the
hidden .virtfs_metadata directory. Directories exported by this security model cannot
interact with other unix tools. "none" security model is same as
passthrough except the sever won't report failures if it fails to
set file attributes like ownership. Security model is mandatory only
for local fsdriver. Other fsdrivers (like proxy) don't take security
model as a parameter.
@item writeout=@var{writeout}
This is an optional argument. The only supported value is "immediate".
This means that host page cache will be used to read and write data but
write notification will be sent to the guest only when the data has been
reported as written by the storage subsystem.
@item readonly
Enables exporting 9p share as a readonly mount for guests. By default
read-write access is given.
@item socket=@var{socket}
Enables proxy filesystem driver to use passed socket file for
communicating with virtfs-proxy-helper(1). Usually a helper like libvirt
will create socketpair and pass one of the fds as sock_fd.
@item sock_fd
Enables proxy filesystem driver to use passed 'sock_fd' as the socket
descriptor for interfacing with virtfs-proxy-helper(1).
@item fmode=@var{fmode}
Specifies the default mode for newly created files on the host. Works only
with security models "mapped-xattr" and "mapped-file".
@item dmode=@var{dmode}
Specifies the default mode for newly created directories on the host. Works
only with security models "mapped-xattr" and "mapped-file".
@item mount_tag=@var{mount_tag}
Specifies the tag name to be used by the guest to mount this export point.
@item multidevs=@var{multidevs}
Specifies how to deal with multiple devices being shared with a 9p export.
Supported behaviours are either "remap", "forbid" or "warn". The latter is
the default behaviour on which virtfs 9p expects only one device to be
shared with the same export, and if more than one device is shared and
accessed via the same 9p export then only a warning message is logged
(once) by qemu on host side. In order to avoid file ID collisions on guest
you should either create a separate virtfs export for each device to be
shared with guests (recommended way) or you might use "remap" instead which
allows you to share multiple devices with only one export instead, which is
achieved by remapping the original inode numbers from host to guest in a
way that would prevent such collisions. Remapping inodes in such use cases
is required because the original device IDs from host are never passed and
exposed on guest. Instead all files of an export shared with virtfs always
share the same device id on guest. So two files with identical inode
numbers but from actually different devices on host would otherwise cause a
file ID collision and hence potential misbehaviours on guest. "forbid" on
the other hand assumes like "warn" that only one device is shared by the
same export, however it will not only log a warning message but also
deny access to additional devices on guest. Note though that "forbid" does
currently not block all possible file access operations (e.g. readdir()
would still return entries from other devices).
@end table
ETEXI

DEF("virtfs_synth", 0, QEMU_OPTION_virtfs_synth,
    "-virtfs_synth Create synthetic file system image\n",
    QEMU_ARCH_ALL)
STEXI
@item -virtfs_synth
@findex -virtfs_synth
Create synthetic file system image. Note that this option is now deprecated.
Please use @code{-fsdev synth} and @code{-device virtio-9p-...} instead.
ETEXI

DEF("iscsi", HAS_ARG, QEMU_OPTION_iscsi,
    "-iscsi [user=user][,password=password]\n"
    "       [,header-digest=CRC32C|CR32C-NONE|NONE-CRC32C|NONE\n"
    "       [,initiator-name=initiator-iqn][,id=target-iqn]\n"
    "       [,timeout=timeout]\n"
    "                iSCSI session parameters\n", QEMU_ARCH_ALL)

STEXI
@item -iscsi
@findex -iscsi
Configure iSCSI session parameters.
ETEXI

STEXI
@end table
ETEXI
DEFHEADING()

DEFHEADING(USB options:)
STEXI
@table @option
ETEXI

DEF("usb", 0, QEMU_OPTION_usb,
    "-usb            enable on-board USB host controller (if not enabled by default)\n",
    QEMU_ARCH_ALL)
STEXI
@item -usb
@findex -usb
Enable USB emulation on machine types with an on-board USB host controller (if
not enabled by default).  Note that on-board USB host controllers may not
support USB 3.0.  In this case @option{-device qemu-xhci} can be used instead
on machines with PCI.
ETEXI

DEF("usbdevice", HAS_ARG, QEMU_OPTION_usbdevice,
    "-usbdevice name add the host or guest USB device 'name'\n",
    QEMU_ARCH_ALL)
STEXI

@item -usbdevice @var{devname}
@findex -usbdevice
Add the USB device @var{devname}. Note that this option is deprecated,
please use @code{-device usb-...} instead. @xref{usb_devices}.

@table @option

@item mouse
Virtual Mouse. This will override the PS/2 mouse emulation when activated.

@item tablet
Pointer device that uses absolute coordinates (like a touchscreen). This
means QEMU is able to report the mouse position without having to grab the
mouse. Also overrides the PS/2 mouse emulation when activated.

@item braille
Braille device.  This will use BrlAPI to display the braille output on a real
or fake device.

@end table
ETEXI

STEXI
@end table
ETEXI
DEFHEADING()

DEFHEADING(Display options:)
STEXI
@table @option
ETEXI

DEF("display", HAS_ARG, QEMU_OPTION_display,
#if defined(CONFIG_SPICE)
    "-display spice-app[,gl=on|off]\n"
#endif
#if defined(CONFIG_SDL)
    "-display sdl[,alt_grab=on|off][,ctrl_grab=on|off]\n"
    "            [,window_close=on|off][,gl=on|core|es|off]\n"
#endif
#if defined(CONFIG_GTK)
    "-display gtk[,grab_on_hover=on|off][,gl=on|off]|\n"
#endif
#if defined(CONFIG_VNC)
    "-display vnc=<display>[,<optargs>]\n"
#endif
#if defined(CONFIG_CURSES)
    "-display curses[,charset=<encoding>]\n"
#endif
#if defined(CONFIG_OPENGL)
    "-display egl-headless[,rendernode=<file>]\n"
#endif
    "-display none\n"
    "                select display backend type\n"
    "                The default display is equivalent to\n                "
#if defined(CONFIG_GTK)
            "\"-display gtk\"\n"
#elif defined(CONFIG_SDL)
            "\"-display sdl\"\n"
#elif defined(CONFIG_COCOA)
            "\"-display cocoa\"\n"
#elif defined(CONFIG_VNC)
            "\"-vnc localhost:0,to=99,id=default\"\n"
#else
            "\"-display none\"\n"
#endif
    , QEMU_ARCH_ALL)
STEXI
@item -display @var{type}
@findex -display
Select type of display to use. This option is a replacement for the
old style -sdl/-curses/... options. Valid values for @var{type} are
@table @option
@item sdl
Display video output via SDL (usually in a separate graphics
window; see the SDL documentation for other possibilities).
@item curses
Display video output via curses. For graphics device models which
support a text mode, QEMU can display this output using a
curses/ncurses interface. Nothing is displayed when the graphics
device is in graphical mode or if the graphics device does not support
a text mode. Generally only the VGA device models support text mode.
The font charset used by the guest can be specified with the
@code{charset} option, for example @code{charset=CP850} for IBM CP850
encoding. The default is @code{CP437}.
@item none
Do not display video output. The guest will still see an emulated
graphics card, but its output will not be displayed to the QEMU
user. This option differs from the -nographic option in that it
only affects what is done with video output; -nographic also changes
the destination of the serial and parallel port data.
@item gtk
Display video output in a GTK window. This interface provides drop-down
menus and other UI elements to configure and control the VM during
runtime.
@item vnc
Start a VNC server on display <arg>
@item egl-headless
Offload all OpenGL operations to a local DRI device. For any graphical display,
this display needs to be paired with either VNC or SPICE displays.
@item spice-app
Start QEMU as a Spice server and launch the default Spice client
application. The Spice server will redirect the serial consoles and
QEMU monitors. (Since 4.0)
@end table
ETEXI

DEF("nographic", 0, QEMU_OPTION_nographic,
    "-nographic      disable graphical output and redirect serial I/Os to console\n",
    QEMU_ARCH_ALL)
STEXI
@item -nographic
@findex -nographic
Normally, if QEMU is compiled with graphical window support, it displays
output such as guest graphics, guest console, and the QEMU monitor in a
window. With this option, you can totally disable graphical output so
that QEMU is a simple command line application. The emulated serial port
is redirected on the console and muxed with the monitor (unless
redirected elsewhere explicitly). Therefore, you can still use QEMU to
debug a Linux kernel with a serial console. Use @key{C-a h} for help on
switching between the console and monitor.
ETEXI

DEF("curses", 0, QEMU_OPTION_curses,
    "-curses         shorthand for -display curses\n",
    QEMU_ARCH_ALL)
STEXI
@item -curses
@findex -curses
Normally, if QEMU is compiled with graphical window support, it displays
output such as guest graphics, guest console, and the QEMU monitor in a
window. With this option, QEMU can display the VGA output when in text
mode using a curses/ncurses interface. Nothing is displayed in graphical
mode.
ETEXI

DEF("alt-grab", 0, QEMU_OPTION_alt_grab,
    "-alt-grab       use Ctrl-Alt-Shift to grab mouse (instead of Ctrl-Alt)\n",
    QEMU_ARCH_ALL)
STEXI
@item -alt-grab
@findex -alt-grab
Use Ctrl-Alt-Shift to grab mouse (instead of Ctrl-Alt). Note that this also
affects the special keys (for fullscreen, monitor-mode switching, etc).
ETEXI

DEF("ctrl-grab", 0, QEMU_OPTION_ctrl_grab,
    "-ctrl-grab      use Right-Ctrl to grab mouse (instead of Ctrl-Alt)\n",
    QEMU_ARCH_ALL)
STEXI
@item -ctrl-grab
@findex -ctrl-grab
Use Right-Ctrl to grab mouse (instead of Ctrl-Alt). Note that this also
affects the special keys (for fullscreen, monitor-mode switching, etc).
ETEXI

DEF("no-quit", 0, QEMU_OPTION_no_quit,
    "-no-quit        disable SDL window close capability\n", QEMU_ARCH_ALL)
STEXI
@item -no-quit
@findex -no-quit
Disable SDL window close capability.
ETEXI

DEF("sdl", 0, QEMU_OPTION_sdl,
    "-sdl            shorthand for -display sdl\n", QEMU_ARCH_ALL)
STEXI
@item -sdl
@findex -sdl
Enable SDL.
ETEXI

DEF("spice", HAS_ARG, QEMU_OPTION_spice,
    "-spice [port=port][,tls-port=secured-port][,x509-dir=<dir>]\n"
    "       [,x509-key-file=<file>][,x509-key-password=<file>]\n"
    "       [,x509-cert-file=<file>][,x509-cacert-file=<file>]\n"
    "       [,x509-dh-key-file=<file>][,addr=addr][,ipv4|ipv6|unix]\n"
    "       [,tls-ciphers=<list>]\n"
    "       [,tls-channel=[main|display|cursor|inputs|record|playback]]\n"
    "       [,plaintext-channel=[main|display|cursor|inputs|record|playback]]\n"
    "       [,sasl][,password=<secret>][,disable-ticketing]\n"
    "       [,image-compression=[auto_glz|auto_lz|quic|glz|lz|off]]\n"
    "       [,jpeg-wan-compression=[auto|never|always]]\n"
    "       [,zlib-glz-wan-compression=[auto|never|always]]\n"
    "       [,streaming-video=[off|all|filter]][,disable-copy-paste]\n"
    "       [,disable-agent-file-xfer][,agent-mouse=[on|off]]\n"
    "       [,playback-compression=[on|off]][,seamless-migration=[on|off]]\n"
    "       [,gl=[on|off]][,rendernode=<file>]\n"
    "   enable spice\n"
    "   at least one of {port, tls-port} is mandatory\n",
    QEMU_ARCH_ALL)
STEXI
@item -spice @var{option}[,@var{option}[,...]]
@findex -spice
Enable the spice remote desktop protocol. Valid options are

@table @option

@item port=<nr>
Set the TCP port spice is listening on for plaintext channels.

@item addr=<addr>
Set the IP address spice is listening on.  Default is any address.

@item ipv4
@itemx ipv6
@itemx unix
Force using the specified IP version.

@item password=<secret>
Set the password you need to authenticate.

@item sasl
Require that the client use SASL to authenticate with the spice.
The exact choice of authentication method used is controlled from the
system / user's SASL configuration file for the 'qemu' service. This
is typically found in /etc/sasl2/qemu.conf. If running QEMU as an
unprivileged user, an environment variable SASL_CONF_PATH can be used
to make it search alternate locations for the service config.
While some SASL auth methods can also provide data encryption (eg GSSAPI),
it is recommended that SASL always be combined with the 'tls' and
'x509' settings to enable use of SSL and server certificates. This
ensures a data encryption preventing compromise of authentication
credentials.

@item disable-ticketing
Allow client connects without authentication.

@item disable-copy-paste
Disable copy paste between the client and the guest.

@item disable-agent-file-xfer
Disable spice-vdagent based file-xfer between the client and the guest.

@item tls-port=<nr>
Set the TCP port spice is listening on for encrypted channels.

@item x509-dir=<dir>
Set the x509 file directory. Expects same filenames as -vnc $display,x509=$dir

@item x509-key-file=<file>
@itemx x509-key-password=<file>
@itemx x509-cert-file=<file>
@itemx x509-cacert-file=<file>
@itemx x509-dh-key-file=<file>
The x509 file names can also be configured individually.

@item tls-ciphers=<list>
Specify which ciphers to use.

@item tls-channel=[main|display|cursor|inputs|record|playback]
@itemx plaintext-channel=[main|display|cursor|inputs|record|playback]
Force specific channel to be used with or without TLS encryption.  The
options can be specified multiple times to configure multiple
channels.  The special name "default" can be used to set the default
mode.  For channels which are not explicitly forced into one mode the
spice client is allowed to pick tls/plaintext as he pleases.

@item image-compression=[auto_glz|auto_lz|quic|glz|lz|off]
Configure image compression (lossless).
Default is auto_glz.

@item jpeg-wan-compression=[auto|never|always]
@itemx zlib-glz-wan-compression=[auto|never|always]
Configure wan image compression (lossy for slow links).
Default is auto.

@item streaming-video=[off|all|filter]
Configure video stream detection.  Default is off.

@item agent-mouse=[on|off]
Enable/disable passing mouse events via vdagent.  Default is on.

@item playback-compression=[on|off]
Enable/disable audio stream compression (using celt 0.5.1).  Default is on.

@item seamless-migration=[on|off]
Enable/disable spice seamless migration. Default is off.

@item gl=[on|off]
Enable/disable OpenGL context. Default is off.

@item rendernode=<file>
DRM render node for OpenGL rendering. If not specified, it will pick
the first available. (Since 2.9)

@end table
ETEXI

DEF("portrait", 0, QEMU_OPTION_portrait,
    "-portrait       rotate graphical output 90 deg left (only PXA LCD)\n",
    QEMU_ARCH_ALL)
STEXI
@item -portrait
@findex -portrait
Rotate graphical output 90 deg left (only PXA LCD).
ETEXI

DEF("rotate", HAS_ARG, QEMU_OPTION_rotate,
    "-rotate <deg>   rotate graphical output some deg left (only PXA LCD)\n",
    QEMU_ARCH_ALL)
STEXI
@item -rotate @var{deg}
@findex -rotate
Rotate graphical output some deg left (only PXA LCD).
ETEXI

DEF("vga", HAS_ARG, QEMU_OPTION_vga,
    "-vga [std|cirrus|vmware|qxl|xenfb|tcx|cg3|virtio|none]\n"
    "                select video card type\n", QEMU_ARCH_ALL)
STEXI
@item -vga @var{type}
@findex -vga
Select type of VGA card to emulate. Valid values for @var{type} are
@table @option
@item cirrus
Cirrus Logic GD5446 Video card. All Windows versions starting from
Windows 95 should recognize and use this graphic card. For optimal
performances, use 16 bit color depth in the guest and the host OS.
(This card was the default before QEMU 2.2)
@item std
Standard VGA card with Bochs VBE extensions.  If your guest OS
supports the VESA 2.0 VBE extensions (e.g. Windows XP) and if you want
to use high resolution modes (>= 1280x1024x16) then you should use
this option. (This card is the default since QEMU 2.2)
@item vmware
VMWare SVGA-II compatible adapter. Use it if you have sufficiently
recent XFree86/XOrg server or Windows guest with a driver for this
card.
@item qxl
QXL paravirtual graphic card.  It is VGA compatible (including VESA
2.0 VBE support).  Works best with qxl guest drivers installed though.
Recommended choice when using the spice protocol.
@item tcx
(sun4m only) Sun TCX framebuffer. This is the default framebuffer for
sun4m machines and offers both 8-bit and 24-bit colour depths at a
fixed resolution of 1024x768.
@item cg3
(sun4m only) Sun cgthree framebuffer. This is a simple 8-bit framebuffer
for sun4m machines available in both 1024x768 (OpenBIOS) and 1152x900 (OBP)
resolutions aimed at people wishing to run older Solaris versions.
@item virtio
Virtio VGA card.
@item none
Disable VGA card.
@end table
ETEXI

DEF("full-screen", 0, QEMU_OPTION_full_screen,
    "-full-screen    start in full screen\n", QEMU_ARCH_ALL)
STEXI
@item -full-screen
@findex -full-screen
Start in full screen.
ETEXI

DEF("g", 1, QEMU_OPTION_g ,
    "-g WxH[xDEPTH]  Set the initial graphical resolution and depth\n",
    QEMU_ARCH_PPC | QEMU_ARCH_SPARC | QEMU_ARCH_M68K)
STEXI
@item -g @var{width}x@var{height}[x@var{depth}]
@findex -g
Set the initial graphical resolution and depth (PPC, SPARC only).
ETEXI

DEF("vnc", HAS_ARG, QEMU_OPTION_vnc ,
    "-vnc <display>  shorthand for -display vnc=<display>\n", QEMU_ARCH_ALL)
STEXI
@item -vnc @var{display}[,@var{option}[,@var{option}[,...]]]
@findex -vnc
Normally, if QEMU is compiled with graphical window support, it displays
output such as guest graphics, guest console, and the QEMU monitor in a
window. With this option, you can have QEMU listen on VNC display
@var{display} and redirect the VGA display over the VNC session. It is
very useful to enable the usb tablet device when using this option
(option @option{-device usb-tablet}). When using the VNC display, you
must use the @option{-k} parameter to set the keyboard layout if you are
not using en-us. Valid syntax for the @var{display} is

@table @option

@item to=@var{L}

With this option, QEMU will try next available VNC @var{display}s, until the
number @var{L}, if the origianlly defined "-vnc @var{display}" is not
available, e.g. port 5900+@var{display} is already used by another
application. By default, to=0.

@item @var{host}:@var{d}

TCP connections will only be allowed from @var{host} on display @var{d}.
By convention the TCP port is 5900+@var{d}. Optionally, @var{host} can
be omitted in which case the server will accept connections from any host.

@item unix:@var{path}

Connections will be allowed over UNIX domain sockets where @var{path} is the
location of a unix socket to listen for connections on.

@item none

VNC is initialized but not started. The monitor @code{change} command
can be used to later start the VNC server.

@end table

Following the @var{display} value there may be one or more @var{option} flags
separated by commas. Valid options are

@table @option

@item reverse

Connect to a listening VNC client via a ``reverse'' connection. The
client is specified by the @var{display}. For reverse network
connections (@var{host}:@var{d},@code{reverse}), the @var{d} argument
is a TCP port number, not a display number.

@item websocket

Opens an additional TCP listening port dedicated to VNC Websocket connections.
If a bare @var{websocket} option is given, the Websocket port is
5700+@var{display}. An alternative port can be specified with the
syntax @code{websocket}=@var{port}.

If @var{host} is specified connections will only be allowed from this host.
It is possible to control the websocket listen address independently, using
the syntax @code{websocket}=@var{host}:@var{port}.

If no TLS credentials are provided, the websocket connection runs in
unencrypted mode. If TLS credentials are provided, the websocket connection
requires encrypted client connections.

@item password

Require that password based authentication is used for client connections.

The password must be set separately using the @code{set_password} command in
the @ref{pcsys_monitor}. The syntax to change your password is:
@code{set_password <protocol> <password>} where <protocol> could be either
"vnc" or "spice".

If you would like to change <protocol> password expiration, you should use
@code{expire_password <protocol> <expiration-time>} where expiration time could
be one of the following options: now, never, +seconds or UNIX time of
expiration, e.g. +60 to make password expire in 60 seconds, or 1335196800
to make password expire on "Mon Apr 23 12:00:00 EDT 2012" (UNIX time for this
date and time).

You can also use keywords "now" or "never" for the expiration time to
allow <protocol> password to expire immediately or never expire.

@item tls-creds=@var{ID}

Provides the ID of a set of TLS credentials to use to secure the
VNC server. They will apply to both the normal VNC server socket
and the websocket socket (if enabled). Setting TLS credentials
will cause the VNC server socket to enable the VeNCrypt auth
mechanism.  The credentials should have been previously created
using the @option{-object tls-creds} argument.

@item tls-authz=@var{ID}

Provides the ID of the QAuthZ authorization object against which
the client's x509 distinguished name will validated. This object is
only resolved at time of use, so can be deleted and recreated on the
fly while the VNC server is active. If missing, it will default
to denying access.

@item sasl

Require that the client use SASL to authenticate with the VNC server.
The exact choice of authentication method used is controlled from the
system / user's SASL configuration file for the 'qemu' service. This
is typically found in /etc/sasl2/qemu.conf. If running QEMU as an
unprivileged user, an environment variable SASL_CONF_PATH can be used
to make it search alternate locations for the service config.
While some SASL auth methods can also provide data encryption (eg GSSAPI),
it is recommended that SASL always be combined with the 'tls' and
'x509' settings to enable use of SSL and server certificates. This
ensures a data encryption preventing compromise of authentication
credentials. See the @ref{vnc_security} section for details on using
SASL authentication.

@item sasl-authz=@var{ID}

Provides the ID of the QAuthZ authorization object against which
the client's SASL username will validated. This object is
only resolved at time of use, so can be deleted and recreated on the
fly while the VNC server is active. If missing, it will default
to denying access.

@item acl

Legacy method for enabling authorization of clients against the
x509 distinguished name and SASL username. It results in the creation
of two @code{authz-list} objects with IDs of @code{vnc.username} and
@code{vnc.x509dname}. The rules for these objects must be configured
with the HMP ACL commands.

This option is deprecated and should no longer be used. The new
@option{sasl-authz} and @option{tls-authz} options are a
replacement.

@item lossy

Enable lossy compression methods (gradient, JPEG, ...). If this
option is set, VNC client may receive lossy framebuffer updates
depending on its encoding settings. Enabling this option can save
a lot of bandwidth at the expense of quality.

@item non-adaptive

Disable adaptive encodings. Adaptive encodings are enabled by default.
An adaptive encoding will try to detect frequently updated screen regions,
and send updates in these regions using a lossy encoding (like JPEG).
This can be really helpful to save bandwidth when playing videos. Disabling
adaptive encodings restores the original static behavior of encodings
like Tight.

@item share=[allow-exclusive|force-shared|ignore]

Set display sharing policy.  'allow-exclusive' allows clients to ask
for exclusive access.  As suggested by the rfb spec this is
implemented by dropping other connections.  Connecting multiple
clients in parallel requires all clients asking for a shared session
(vncviewer: -shared switch).  This is the default.  'force-shared'
disables exclusive client access.  Useful for shared desktop sessions,
where you don't want someone forgetting specify -shared disconnect
everybody else.  'ignore' completely ignores the shared flag and
allows everybody connect unconditionally.  Doesn't conform to the rfb
spec but is traditional QEMU behavior.

@item key-delay-ms

Set keyboard delay, for key down and key up events, in milliseconds.
Default is 10.  Keyboards are low-bandwidth devices, so this slowdown
can help the device and guest to keep up and not lose events in case
events are arriving in bulk.  Possible causes for the latter are flaky
network connections, or scripts for automated testing.

@item audiodev=@var{audiodev}

Use the specified @var{audiodev} when the VNC client requests audio
transmission. When not using an -audiodev argument, this option must
be omitted, otherwise is must be present and specify a valid audiodev.

@end table
ETEXI

STEXI
@end table
ETEXI
ARCHHEADING(, QEMU_ARCH_I386)

ARCHHEADING(i386 target only:, QEMU_ARCH_I386)
STEXI
@table @option
ETEXI

DEF("win2k-hack", 0, QEMU_OPTION_win2k_hack,
    "-win2k-hack     use it when installing Windows 2000 to avoid a disk full bug\n",
    QEMU_ARCH_I386)
STEXI
@item -win2k-hack
@findex -win2k-hack
Use it when installing Windows 2000 to avoid a disk full bug. After
Windows 2000 is installed, you no longer need this option (this option
slows down the IDE transfers).
ETEXI

DEF("no-fd-bootchk", 0, QEMU_OPTION_no_fd_bootchk,
    "-no-fd-bootchk  disable boot signature checking for floppy disks\n",
    QEMU_ARCH_I386)
STEXI
@item -no-fd-bootchk
@findex -no-fd-bootchk
Disable boot signature checking for floppy disks in BIOS. May
be needed to boot from old floppy disks.
ETEXI

DEF("no-acpi", 0, QEMU_OPTION_no_acpi,
           "-no-acpi        disable ACPI\n", QEMU_ARCH_I386 | QEMU_ARCH_ARM)
STEXI
@item -no-acpi
@findex -no-acpi
Disable ACPI (Advanced Configuration and Power Interface) support. Use
it if your guest OS complains about ACPI problems (PC target machine
only).
ETEXI

DEF("no-hpet", 0, QEMU_OPTION_no_hpet,
    "-no-hpet        disable HPET\n", QEMU_ARCH_I386)
STEXI
@item -no-hpet
@findex -no-hpet
Disable HPET support.
ETEXI

DEF("acpitable", HAS_ARG, QEMU_OPTION_acpitable,
    "-acpitable [sig=str][,rev=n][,oem_id=str][,oem_table_id=str][,oem_rev=n][,asl_compiler_id=str][,asl_compiler_rev=n][,{data|file}=file1[:file2]...]\n"
    "                ACPI table description\n", QEMU_ARCH_I386)
STEXI
@item -acpitable [sig=@var{str}][,rev=@var{n}][,oem_id=@var{str}][,oem_table_id=@var{str}][,oem_rev=@var{n}] [,asl_compiler_id=@var{str}][,asl_compiler_rev=@var{n}][,data=@var{file1}[:@var{file2}]...]
@findex -acpitable
Add ACPI table with specified header fields and context from specified files.
For file=, take whole ACPI table from the specified files, including all
ACPI headers (possible overridden by other options).
For data=, only data
portion of the table is used, all header information is specified in the
command line.
If a SLIC table is supplied to QEMU, then the SLIC's oem_id and oem_table_id
fields will override the same in the RSDT and the FADT (a.k.a. FACP), in order
to ensure the field matches required by the Microsoft SLIC spec and the ACPI
spec.
ETEXI

DEF("smbios", HAS_ARG, QEMU_OPTION_smbios,
    "-smbios file=binary\n"
    "                load SMBIOS entry from binary file\n"
    "-smbios type=0[,vendor=str][,version=str][,date=str][,release=%d.%d]\n"
    "              [,uefi=on|off]\n"
    "                specify SMBIOS type 0 fields\n"
    "-smbios type=1[,manufacturer=str][,product=str][,version=str][,serial=str]\n"
    "              [,uuid=uuid][,sku=str][,family=str]\n"
    "                specify SMBIOS type 1 fields\n"
    "-smbios type=2[,manufacturer=str][,product=str][,version=str][,serial=str]\n"
    "              [,asset=str][,location=str]\n"
    "                specify SMBIOS type 2 fields\n"
    "-smbios type=3[,manufacturer=str][,version=str][,serial=str][,asset=str]\n"
    "              [,sku=str]\n"
    "                specify SMBIOS type 3 fields\n"
    "-smbios type=4[,sock_pfx=str][,manufacturer=str][,version=str][,serial=str]\n"
    "              [,asset=str][,part=str]\n"
    "                specify SMBIOS type 4 fields\n"
    "-smbios type=17[,loc_pfx=str][,bank=str][,manufacturer=str][,serial=str]\n"
    "               [,asset=str][,part=str][,speed=%d]\n"
    "                specify SMBIOS type 17 fields\n",
    QEMU_ARCH_I386 | QEMU_ARCH_ARM)
STEXI
@item -smbios file=@var{binary}
@findex -smbios
Load SMBIOS entry from binary file.

@item -smbios type=0[,vendor=@var{str}][,version=@var{str}][,date=@var{str}][,release=@var{%d.%d}][,uefi=on|off]
Specify SMBIOS type 0 fields

@item -smbios type=1[,manufacturer=@var{str}][,product=@var{str}][,version=@var{str}][,serial=@var{str}][,uuid=@var{uuid}][,sku=@var{str}][,family=@var{str}]
Specify SMBIOS type 1 fields

@item -smbios type=2[,manufacturer=@var{str}][,product=@var{str}][,version=@var{str}][,serial=@var{str}][,asset=@var{str}][,location=@var{str}]
Specify SMBIOS type 2 fields

@item -smbios type=3[,manufacturer=@var{str}][,version=@var{str}][,serial=@var{str}][,asset=@var{str}][,sku=@var{str}]
Specify SMBIOS type 3 fields

@item -smbios type=4[,sock_pfx=@var{str}][,manufacturer=@var{str}][,version=@var{str}][,serial=@var{str}][,asset=@var{str}][,part=@var{str}]
Specify SMBIOS type 4 fields

@item -smbios type=17[,loc_pfx=@var{str}][,bank=@var{str}][,manufacturer=@var{str}][,serial=@var{str}][,asset=@var{str}][,part=@var{str}][,speed=@var{%d}]
Specify SMBIOS type 17 fields
ETEXI

STEXI
@end table
ETEXI
DEFHEADING()

DEFHEADING(Network options:)
STEXI
@table @option
ETEXI

DEF("netdev", HAS_ARG, QEMU_OPTION_netdev,
#ifdef CONFIG_SLIRP
    "-netdev user,id=str[,ipv4[=on|off]][,net=addr[/mask]][,host=addr]\n"
    "         [,ipv6[=on|off]][,ipv6-net=addr[/int]][,ipv6-host=addr]\n"
    "         [,restrict=on|off][,hostname=host][,dhcpstart=addr]\n"
    "         [,dns=addr][,ipv6-dns=addr][,dnssearch=domain][,domainname=domain]\n"
    "         [,tftp=dir][,tftp-server-name=name][,bootfile=f][,hostfwd=rule][,guestfwd=rule]"
#ifndef _WIN32
                                             "[,smb=dir[,smbserver=addr]]\n"
#endif
    "                configure a user mode network backend with ID 'str',\n"
    "                its DHCP server and optional services\n"
#endif
#ifdef _WIN32
    "-netdev tap,id=str,ifname=name\n"
    "                configure a host TAP network backend with ID 'str'\n"
#else
    "-netdev tap,id=str[,fd=h][,fds=x:y:...:z][,ifname=name][,script=file][,downscript=dfile]\n"
    "         [,br=bridge][,helper=helper][,sndbuf=nbytes][,vnet_hdr=on|off][,vhost=on|off]\n"
    "         [,vhostfd=h][,vhostfds=x:y:...:z][,vhostforce=on|off][,queues=n]\n"
    "         [,poll-us=n]\n"
    "                configure a host TAP network backend with ID 'str'\n"
    "                connected to a bridge (default=" DEFAULT_BRIDGE_INTERFACE ")\n"
    "                use network scripts 'file' (default=" DEFAULT_NETWORK_SCRIPT ")\n"
    "                to configure it and 'dfile' (default=" DEFAULT_NETWORK_DOWN_SCRIPT ")\n"
    "                to deconfigure it\n"
    "                use '[down]script=no' to disable script execution\n"
    "                use network helper 'helper' (default=" DEFAULT_BRIDGE_HELPER ") to\n"
    "                configure it\n"
    "                use 'fd=h' to connect to an already opened TAP interface\n"
    "                use 'fds=x:y:...:z' to connect to already opened multiqueue capable TAP interfaces\n"
    "                use 'sndbuf=nbytes' to limit the size of the send buffer (the\n"
    "                default is disabled 'sndbuf=0' to enable flow control set 'sndbuf=1048576')\n"
    "                use vnet_hdr=off to avoid enabling the IFF_VNET_HDR tap flag\n"
    "                use vnet_hdr=on to make the lack of IFF_VNET_HDR support an error condition\n"
    "                use vhost=on to enable experimental in kernel accelerator\n"
    "                    (only has effect for virtio guests which use MSIX)\n"
    "                use vhostforce=on to force vhost on for non-MSIX virtio guests\n"
    "                use 'vhostfd=h' to connect to an already opened vhost net device\n"
    "                use 'vhostfds=x:y:...:z to connect to multiple already opened vhost net devices\n"
    "                use 'queues=n' to specify the number of queues to be created for multiqueue TAP\n"
    "                use 'poll-us=n' to speciy the maximum number of microseconds that could be\n"
    "                spent on busy polling for vhost net\n"
    "-netdev bridge,id=str[,br=bridge][,helper=helper]\n"
    "                configure a host TAP network backend with ID 'str' that is\n"
    "                connected to a bridge (default=" DEFAULT_BRIDGE_INTERFACE ")\n"
    "                using the program 'helper (default=" DEFAULT_BRIDGE_HELPER ")\n"
#endif
#ifdef __linux__
    "-netdev l2tpv3,id=str,src=srcaddr,dst=dstaddr[,srcport=srcport][,dstport=dstport]\n"
    "         [,rxsession=rxsession],txsession=txsession[,ipv6=on/off][,udp=on/off]\n"
    "         [,cookie64=on/off][,counter][,pincounter][,txcookie=txcookie]\n"
    "         [,rxcookie=rxcookie][,offset=offset]\n"
    "                configure a network backend with ID 'str' connected to\n"
    "                an Ethernet over L2TPv3 pseudowire.\n"
    "                Linux kernel 3.3+ as well as most routers can talk\n"
    "                L2TPv3. This transport allows connecting a VM to a VM,\n"
    "                VM to a router and even VM to Host. It is a nearly-universal\n"
    "                standard (RFC3391). Note - this implementation uses static\n"
    "                pre-configured tunnels (same as the Linux kernel).\n"
    "                use 'src=' to specify source address\n"
    "                use 'dst=' to specify destination address\n"
    "                use 'udp=on' to specify udp encapsulation\n"
    "                use 'srcport=' to specify source udp port\n"
    "                use 'dstport=' to specify destination udp port\n"
    "                use 'ipv6=on' to force v6\n"
    "                L2TPv3 uses cookies to prevent misconfiguration as\n"
    "                well as a weak security measure\n"
    "                use 'rxcookie=0x012345678' to specify a rxcookie\n"
    "                use 'txcookie=0x012345678' to specify a txcookie\n"
    "                use 'cookie64=on' to set cookie size to 64 bit, otherwise 32\n"
    "                use 'counter=off' to force a 'cut-down' L2TPv3 with no counter\n"
    "                use 'pincounter=on' to work around broken counter handling in peer\n"
    "                use 'offset=X' to add an extra offset between header and data\n"
#endif
    "-netdev socket,id=str[,fd=h][,listen=[host]:port][,connect=host:port]\n"
    "                configure a network backend to connect to another network\n"
    "                using a socket connection\n"
    "-netdev socket,id=str[,fd=h][,mcast=maddr:port[,localaddr=addr]]\n"
    "                configure a network backend to connect to a multicast maddr and port\n"
    "                use 'localaddr=addr' to specify the host address to send packets from\n"
    "-netdev socket,id=str[,fd=h][,udp=host:port][,localaddr=host:port]\n"
    "                configure a network backend to connect to another network\n"
    "                using an UDP tunnel\n"
#ifdef CONFIG_VDE
    "-netdev vde,id=str[,sock=socketpath][,port=n][,group=groupname][,mode=octalmode]\n"
    "                configure a network backend to connect to port 'n' of a vde switch\n"
    "                running on host and listening for incoming connections on 'socketpath'.\n"
    "                Use group 'groupname' and mode 'octalmode' to change default\n"
    "                ownership and permissions for communication port.\n"
#endif
#ifdef CONFIG_NETMAP
    "-netdev netmap,id=str,ifname=name[,devname=nmname]\n"
    "                attach to the existing netmap-enabled network interface 'name', or to a\n"
    "                VALE port (created on the fly) called 'name' ('nmname' is name of the \n"
    "                netmap device, defaults to '/dev/netmap')\n"
#endif
#ifdef CONFIG_POSIX
    "-netdev vhost-user,id=str,chardev=dev[,vhostforce=on|off]\n"
    "                configure a vhost-user network, backed by a chardev 'dev'\n"
#endif
    "-netdev hubport,id=str,hubid=n[,netdev=nd]\n"
    "                configure a hub port on the hub with ID 'n'\n", QEMU_ARCH_ALL)
DEF("nic", HAS_ARG, QEMU_OPTION_nic,
    "-nic [tap|bridge|"
#ifdef CONFIG_SLIRP
    "user|"
#endif
#ifdef __linux__
    "l2tpv3|"
#endif
#ifdef CONFIG_VDE
    "vde|"
#endif
#ifdef CONFIG_NETMAP
    "netmap|"
#endif
#ifdef CONFIG_POSIX
    "vhost-user|"
#endif
    "socket][,option][,...][mac=macaddr]\n"
    "                initialize an on-board / default host NIC (using MAC address\n"
    "                macaddr) and connect it to the given host network backend\n"
    "-nic none       use it alone to have zero network devices (the default is to\n"
    "                provided a 'user' network connection)\n",
    QEMU_ARCH_ALL)
DEF("net", HAS_ARG, QEMU_OPTION_net,
    "-net nic[,macaddr=mac][,model=type][,name=str][,addr=str][,vectors=v]\n"
    "                configure or create an on-board (or machine default) NIC and\n"
    "                connect it to hub 0 (please use -nic unless you need a hub)\n"
    "-net ["
#ifdef CONFIG_SLIRP
    "user|"
#endif
    "tap|"
    "bridge|"
#ifdef CONFIG_VDE
    "vde|"
#endif
#ifdef CONFIG_NETMAP
    "netmap|"
#endif
    "socket][,option][,option][,...]\n"
    "                old way to initialize a host network interface\n"
    "                (use the -netdev option if possible instead)\n", QEMU_ARCH_ALL)
STEXI
@item -nic [tap|bridge|user|l2tpv3|vde|netmap|vhost-user|socket][,...][,mac=macaddr][,model=mn]
@findex -nic
This option is a shortcut for configuring both the on-board (default) guest
NIC hardware and the host network backend in one go. The host backend options
are the same as with the corresponding @option{-netdev} options below.
The guest NIC model can be set with @option{model=@var{modelname}}.
Use @option{model=help} to list the available device types.
The hardware MAC address can be set with @option{mac=@var{macaddr}}.

The following two example do exactly the same, to show how @option{-nic} can
be used to shorten the command line length (note that the e1000 is the default
on i386, so the @option{model=e1000} parameter could even be omitted here, too):
@example
@value{qemu_system} -netdev user,id=n1,ipv6=off -device e1000,netdev=n1,mac=52:54:98:76:54:32
@value{qemu_system} -nic user,ipv6=off,model=e1000,mac=52:54:98:76:54:32
@end example

@item -nic none
Indicate that no network devices should be configured. It is used to override
the default configuration (default NIC with ``user'' host network backend)
which is activated if no other networking options are provided.

@item -netdev user,id=@var{id}[,@var{option}][,@var{option}][,...]
@findex -netdev
Configure user mode host network backend which requires no administrator
privilege to run. Valid options are:

@table @option
@item id=@var{id}
Assign symbolic name for use in monitor commands.

@item ipv4=on|off and ipv6=on|off
Specify that either IPv4 or IPv6 must be enabled. If neither is specified
both protocols are enabled.

@item net=@var{addr}[/@var{mask}]
Set IP network address the guest will see. Optionally specify the netmask,
either in the form a.b.c.d or as number of valid top-most bits. Default is
10.0.2.0/24.

@item host=@var{addr}
Specify the guest-visible address of the host. Default is the 2nd IP in the
guest network, i.e. x.x.x.2.

@item ipv6-net=@var{addr}[/@var{int}]
Set IPv6 network address the guest will see (default is fec0::/64). The
network prefix is given in the usual hexadecimal IPv6 address
notation. The prefix size is optional, and is given as the number of
valid top-most bits (default is 64).

@item ipv6-host=@var{addr}
Specify the guest-visible IPv6 address of the host. Default is the 2nd IPv6 in
the guest network, i.e. xxxx::2.

@item restrict=on|off
If this option is enabled, the guest will be isolated, i.e. it will not be
able to contact the host and no guest IP packets will be routed over the host
to the outside. This option does not affect any explicitly set forwarding rules.

@item hostname=@var{name}
Specifies the client hostname reported by the built-in DHCP server.

@item dhcpstart=@var{addr}
Specify the first of the 16 IPs the built-in DHCP server can assign. Default
is the 15th to 31st IP in the guest network, i.e. x.x.x.15 to x.x.x.31.

@item dns=@var{addr}
Specify the guest-visible address of the virtual nameserver. The address must
be different from the host address. Default is the 3rd IP in the guest network,
i.e. x.x.x.3.

@item ipv6-dns=@var{addr}
Specify the guest-visible address of the IPv6 virtual nameserver. The address
must be different from the host address. Default is the 3rd IP in the guest
network, i.e. xxxx::3.

@item dnssearch=@var{domain}
Provides an entry for the domain-search list sent by the built-in
DHCP server. More than one domain suffix can be transmitted by specifying
this option multiple times. If supported, this will cause the guest to
automatically try to append the given domain suffix(es) in case a domain name
can not be resolved.

Example:
@example
@value{qemu_system} -nic user,dnssearch=mgmt.example.org,dnssearch=example.org
@end example

@item domainname=@var{domain}
Specifies the client domain name reported by the built-in DHCP server.

@item tftp=@var{dir}
When using the user mode network stack, activate a built-in TFTP
server. The files in @var{dir} will be exposed as the root of a TFTP server.
The TFTP client on the guest must be configured in binary mode (use the command
@code{bin} of the Unix TFTP client).

@item tftp-server-name=@var{name}
In BOOTP reply, broadcast @var{name} as the "TFTP server name" (RFC2132 option
66). This can be used to advise the guest to load boot files or configurations
from a different server than the host address.

@item bootfile=@var{file}
When using the user mode network stack, broadcast @var{file} as the BOOTP
filename. In conjunction with @option{tftp}, this can be used to network boot
a guest from a local directory.

Example (using pxelinux):
@example
@value{qemu_system} -hda linux.img -boot n -device e1000,netdev=n1 \
    -netdev user,id=n1,tftp=/path/to/tftp/files,bootfile=/pxelinux.0
@end example

@item smb=@var{dir}[,smbserver=@var{addr}]
When using the user mode network stack, activate a built-in SMB
server so that Windows OSes can access to the host files in @file{@var{dir}}
transparently. The IP address of the SMB server can be set to @var{addr}. By
default the 4th IP in the guest network is used, i.e. x.x.x.4.

In the guest Windows OS, the line:
@example
10.0.2.4 smbserver
@end example
must be added in the file @file{C:\WINDOWS\LMHOSTS} (for windows 9x/Me)
or @file{C:\WINNT\SYSTEM32\DRIVERS\ETC\LMHOSTS} (Windows NT/2000).

Then @file{@var{dir}} can be accessed in @file{\\smbserver\qemu}.

Note that a SAMBA server must be installed on the host OS.

@item hostfwd=[tcp|udp]:[@var{hostaddr}]:@var{hostport}-[@var{guestaddr}]:@var{guestport}
Redirect incoming TCP or UDP connections to the host port @var{hostport} to
the guest IP address @var{guestaddr} on guest port @var{guestport}. If
@var{guestaddr} is not specified, its value is x.x.x.15 (default first address
given by the built-in DHCP server). By specifying @var{hostaddr}, the rule can
be bound to a specific host interface. If no connection type is set, TCP is
used. This option can be given multiple times.

For example, to redirect host X11 connection from screen 1 to guest
screen 0, use the following:

@example
# on the host
@value{qemu_system} -nic user,hostfwd=tcp:127.0.0.1:6001-:6000
# this host xterm should open in the guest X11 server
xterm -display :1
@end example

To redirect telnet connections from host port 5555 to telnet port on
the guest, use the following:

@example
# on the host
@value{qemu_system} -nic user,hostfwd=tcp::5555-:23
telnet localhost 5555
@end example

Then when you use on the host @code{telnet localhost 5555}, you
connect to the guest telnet server.

@item guestfwd=[tcp]:@var{server}:@var{port}-@var{dev}
@itemx guestfwd=[tcp]:@var{server}:@var{port}-@var{cmd:command}
Forward guest TCP connections to the IP address @var{server} on port @var{port}
to the character device @var{dev} or to a program executed by @var{cmd:command}
which gets spawned for each connection. This option can be given multiple times.

You can either use a chardev directly and have that one used throughout QEMU's
lifetime, like in the following example:

@example
# open 10.10.1.1:4321 on bootup, connect 10.0.2.100:1234 to it whenever
# the guest accesses it
@value{qemu_system} -nic user,guestfwd=tcp:10.0.2.100:1234-tcp:10.10.1.1:4321
@end example

Or you can execute a command on every TCP connection established by the guest,
so that QEMU behaves similar to an inetd process for that virtual server:

@example
# call "netcat 10.10.1.1 4321" on every TCP connection to 10.0.2.100:1234
# and connect the TCP stream to its stdin/stdout
@value{qemu_system} -nic  'user,id=n1,guestfwd=tcp:10.0.2.100:1234-cmd:netcat 10.10.1.1 4321'
@end example

@end table

@item -netdev tap,id=@var{id}[,fd=@var{h}][,ifname=@var{name}][,script=@var{file}][,downscript=@var{dfile}][,br=@var{bridge}][,helper=@var{helper}]
Configure a host TAP network backend with ID @var{id}.

Use the network script @var{file} to configure it and the network script
@var{dfile} to deconfigure it. If @var{name} is not provided, the OS
automatically provides one. The default network configure script is
@file{/etc/qemu-ifup} and the default network deconfigure script is
@file{/etc/qemu-ifdown}. Use @option{script=no} or @option{downscript=no}
to disable script execution.

If running QEMU as an unprivileged user, use the network helper
@var{helper} to configure the TAP interface and attach it to the bridge.
The default network helper executable is @file{/path/to/qemu-bridge-helper}
and the default bridge device is @file{br0}.

@option{fd}=@var{h} can be used to specify the handle of an already
opened host TAP interface.

Examples:

@example
#launch a QEMU instance with the default network script
@value{qemu_system} linux.img -nic tap
@end example

@example
#launch a QEMU instance with two NICs, each one connected
#to a TAP device
@value{qemu_system} linux.img \
        -netdev tap,id=nd0,ifname=tap0 -device e1000,netdev=nd0 \
        -netdev tap,id=nd1,ifname=tap1 -device rtl8139,netdev=nd1
@end example

@example
#launch a QEMU instance with the default network helper to
#connect a TAP device to bridge br0
@value{qemu_system} linux.img -device virtio-net-pci,netdev=n1 \
        -netdev tap,id=n1,"helper=/path/to/qemu-bridge-helper"
@end example

@item -netdev bridge,id=@var{id}[,br=@var{bridge}][,helper=@var{helper}]
Connect a host TAP network interface to a host bridge device.

Use the network helper @var{helper} to configure the TAP interface and
attach it to the bridge. The default network helper executable is
@file{/path/to/qemu-bridge-helper} and the default bridge
device is @file{br0}.

Examples:

@example
#launch a QEMU instance with the default network helper to
#connect a TAP device to bridge br0
@value{qemu_system} linux.img -netdev bridge,id=n1 -device virtio-net,netdev=n1
@end example

@example
#launch a QEMU instance with the default network helper to
#connect a TAP device to bridge qemubr0
@value{qemu_system} linux.img -netdev bridge,br=qemubr0,id=n1 -device virtio-net,netdev=n1
@end example

@item -netdev socket,id=@var{id}[,fd=@var{h}][,listen=[@var{host}]:@var{port}][,connect=@var{host}:@var{port}]

This host network backend can be used to connect the guest's network to
another QEMU virtual machine using a TCP socket connection. If @option{listen}
is specified, QEMU waits for incoming connections on @var{port}
(@var{host} is optional). @option{connect} is used to connect to
another QEMU instance using the @option{listen} option. @option{fd}=@var{h}
specifies an already opened TCP socket.

Example:
@example
# launch a first QEMU instance
@value{qemu_system} linux.img \
                 -device e1000,netdev=n1,mac=52:54:00:12:34:56 \
                 -netdev socket,id=n1,listen=:1234
# connect the network of this instance to the network of the first instance
@value{qemu_system} linux.img \
                 -device e1000,netdev=n2,mac=52:54:00:12:34:57 \
                 -netdev socket,id=n2,connect=127.0.0.1:1234
@end example

@item -netdev socket,id=@var{id}[,fd=@var{h}][,mcast=@var{maddr}:@var{port}[,localaddr=@var{addr}]]

Configure a socket host network backend to share the guest's network traffic
with another QEMU virtual machines using a UDP multicast socket, effectively
making a bus for every QEMU with same multicast address @var{maddr} and @var{port}.
NOTES:
@enumerate
@item
Several QEMU can be running on different hosts and share same bus (assuming
correct multicast setup for these hosts).
@item
mcast support is compatible with User Mode Linux (argument @option{eth@var{N}=mcast}), see
@url{http://user-mode-linux.sf.net}.
@item
Use @option{fd=h} to specify an already opened UDP multicast socket.
@end enumerate

Example:
@example
# launch one QEMU instance
@value{qemu_system} linux.img \
                 -device e1000,netdev=n1,mac=52:54:00:12:34:56 \
                 -netdev socket,id=n1,mcast=230.0.0.1:1234
# launch another QEMU instance on same "bus"
@value{qemu_system} linux.img \
                 -device e1000,netdev=n2,mac=52:54:00:12:34:57 \
                 -netdev socket,id=n2,mcast=230.0.0.1:1234
# launch yet another QEMU instance on same "bus"
@value{qemu_system} linux.img \
                 -device e1000,netdev=n3,mac=52:54:00:12:34:58 \
                 -netdev socket,id=n3,mcast=230.0.0.1:1234
@end example

Example (User Mode Linux compat.):
@example
# launch QEMU instance (note mcast address selected is UML's default)
@value{qemu_system} linux.img \
                 -device e1000,netdev=n1,mac=52:54:00:12:34:56 \
                 -netdev socket,id=n1,mcast=239.192.168.1:1102
# launch UML
/path/to/linux ubd0=/path/to/root_fs eth0=mcast
@end example

Example (send packets from host's 1.2.3.4):
@example
@value{qemu_system} linux.img \
                 -device e1000,netdev=n1,mac=52:54:00:12:34:56 \
                 -netdev socket,id=n1,mcast=239.192.168.1:1102,localaddr=1.2.3.4
@end example

@item -netdev l2tpv3,id=@var{id},src=@var{srcaddr},dst=@var{dstaddr}[,srcport=@var{srcport}][,dstport=@var{dstport}],txsession=@var{txsession}[,rxsession=@var{rxsession}][,ipv6][,udp][,cookie64][,counter][,pincounter][,txcookie=@var{txcookie}][,rxcookie=@var{rxcookie}][,offset=@var{offset}]
Configure a L2TPv3 pseudowire host network backend. L2TPv3 (RFC3391) is a
popular protocol to transport Ethernet (and other Layer 2) data frames between
two systems. It is present in routers, firewalls and the Linux kernel
(from version 3.3 onwards).

This transport allows a VM to communicate to another VM, router or firewall directly.

@table @option
@item src=@var{srcaddr}
    source address (mandatory)
@item dst=@var{dstaddr}
    destination address (mandatory)
@item udp
    select udp encapsulation (default is ip).
@item srcport=@var{srcport}
    source udp port.
@item dstport=@var{dstport}
    destination udp port.
@item ipv6
    force v6, otherwise defaults to v4.
@item rxcookie=@var{rxcookie}
@itemx txcookie=@var{txcookie}
    Cookies are a weak form of security in the l2tpv3 specification.
Their function is mostly to prevent misconfiguration. By default they are 32
bit.
@item cookie64
    Set cookie size to 64 bit instead of the default 32
@item counter=off
    Force a 'cut-down' L2TPv3 with no counter as in
draft-mkonstan-l2tpext-keyed-ipv6-tunnel-00
@item pincounter=on
    Work around broken counter handling in peer. This may also help on
networks which have packet reorder.
@item offset=@var{offset}
    Add an extra offset between header and data
@end table

For example, to attach a VM running on host 4.3.2.1 via L2TPv3 to the bridge br-lan
on the remote Linux host 1.2.3.4:
@example
# Setup tunnel on linux host using raw ip as encapsulation
# on 1.2.3.4
ip l2tp add tunnel remote 4.3.2.1 local 1.2.3.4 tunnel_id 1 peer_tunnel_id 1 \
    encap udp udp_sport 16384 udp_dport 16384
ip l2tp add session tunnel_id 1 name vmtunnel0 session_id \
    0xFFFFFFFF peer_session_id 0xFFFFFFFF
ifconfig vmtunnel0 mtu 1500
ifconfig vmtunnel0 up
brctl addif br-lan vmtunnel0


# on 4.3.2.1
# launch QEMU instance - if your network has reorder or is very lossy add ,pincounter

@value{qemu_system} linux.img -device e1000,netdev=n1 \
    -netdev l2tpv3,id=n1,src=4.2.3.1,dst=1.2.3.4,udp,srcport=16384,dstport=16384,rxsession=0xffffffff,txsession=0xffffffff,counter

@end example

@item -netdev vde,id=@var{id}[,sock=@var{socketpath}][,port=@var{n}][,group=@var{groupname}][,mode=@var{octalmode}]
Configure VDE backend to connect to PORT @var{n} of a vde switch running on host and
listening for incoming connections on @var{socketpath}. Use GROUP @var{groupname}
and MODE @var{octalmode} to change default ownership and permissions for
communication port. This option is only available if QEMU has been compiled
with vde support enabled.

Example:
@example
# launch vde switch
vde_switch -F -sock /tmp/myswitch
# launch QEMU instance
@value{qemu_system} linux.img -nic vde,sock=/tmp/myswitch
@end example

@item -netdev vhost-user,chardev=@var{id}[,vhostforce=on|off][,queues=n]

Establish a vhost-user netdev, backed by a chardev @var{id}. The chardev should
be a unix domain socket backed one. The vhost-user uses a specifically defined
protocol to pass vhost ioctl replacement messages to an application on the other
end of the socket. On non-MSIX guests, the feature can be forced with
@var{vhostforce}. Use 'queues=@var{n}' to specify the number of queues to
be created for multiqueue vhost-user.

Example:
@example
qemu -m 512 -object memory-backend-file,id=mem,size=512M,mem-path=/hugetlbfs,share=on \
     -numa node,memdev=mem \
     -chardev socket,id=chr0,path=/path/to/socket \
     -netdev type=vhost-user,id=net0,chardev=chr0 \
     -device virtio-net-pci,netdev=net0
@end example

@item -netdev hubport,id=@var{id},hubid=@var{hubid}[,netdev=@var{nd}]

Create a hub port on the emulated hub with ID @var{hubid}.

The hubport netdev lets you connect a NIC to a QEMU emulated hub instead of a
single netdev. Alternatively, you can also connect the hubport to another
netdev with ID @var{nd} by using the @option{netdev=@var{nd}} option.

@item -net nic[,netdev=@var{nd}][,macaddr=@var{mac}][,model=@var{type}] [,name=@var{name}][,addr=@var{addr}][,vectors=@var{v}]
@findex -net
Legacy option to configure or create an on-board (or machine default) Network
Interface Card(NIC) and connect it either to the emulated hub with ID 0 (i.e.
the default hub), or to the netdev @var{nd}.
The NIC is an e1000 by default on the PC target. Optionally, the MAC address
can be changed to @var{mac}, the device address set to @var{addr} (PCI cards
only), and a @var{name} can be assigned for use in monitor commands.
Optionally, for PCI cards, you can specify the number @var{v} of MSI-X vectors
that the card should have; this option currently only affects virtio cards; set
@var{v} = 0 to disable MSI-X. If no @option{-net} option is specified, a single
NIC is created.  QEMU can emulate several different models of network card.
Use @code{-net nic,model=help} for a list of available devices for your target.

@item -net user|tap|bridge|socket|l2tpv3|vde[,...][,name=@var{name}]
Configure a host network backend (with the options corresponding to the same
@option{-netdev} option) and connect it to the emulated hub 0 (the default
hub). Use @var{name} to specify the name of the hub port.
ETEXI

STEXI
@end table
ETEXI
DEFHEADING()

DEFHEADING(Character device options:)

DEF("chardev", HAS_ARG, QEMU_OPTION_chardev,
    "-chardev help\n"
    "-chardev null,id=id[,mux=on|off][,logfile=PATH][,logappend=on|off]\n"
    "-chardev socket,id=id[,host=host],port=port[,to=to][,ipv4][,ipv6][,nodelay][,reconnect=seconds]\n"
    "         [,server][,nowait][,telnet][,websocket][,reconnect=seconds][,mux=on|off]\n"
    "         [,logfile=PATH][,logappend=on|off][,tls-creds=ID][,tls-authz=ID] (tcp)\n"
    "-chardev socket,id=id,path=path[,server][,nowait][,telnet][,websocket][,reconnect=seconds]\n"
    "         [,mux=on|off][,logfile=PATH][,logappend=on|off] (unix)\n"
    "-chardev udp,id=id[,host=host],port=port[,localaddr=localaddr]\n"
    "         [,localport=localport][,ipv4][,ipv6][,mux=on|off]\n"
    "         [,logfile=PATH][,logappend=on|off]\n"
    "-chardev msmouse,id=id[,mux=on|off][,logfile=PATH][,logappend=on|off]\n"
    "-chardev vc,id=id[[,width=width][,height=height]][[,cols=cols][,rows=rows]]\n"
    "         [,mux=on|off][,logfile=PATH][,logappend=on|off]\n"
    "-chardev ringbuf,id=id[,size=size][,logfile=PATH][,logappend=on|off]\n"
    "-chardev file,id=id,path=path[,mux=on|off][,logfile=PATH][,logappend=on|off]\n"
    "-chardev pipe,id=id,path=path[,mux=on|off][,logfile=PATH][,logappend=on|off]\n"
#ifdef _WIN32
    "-chardev console,id=id[,mux=on|off][,logfile=PATH][,logappend=on|off]\n"
    "-chardev serial,id=id,path=path[,mux=on|off][,logfile=PATH][,logappend=on|off]\n"
#else
    "-chardev pty,id=id[,mux=on|off][,logfile=PATH][,logappend=on|off]\n"
    "-chardev stdio,id=id[,mux=on|off][,signal=on|off][,logfile=PATH][,logappend=on|off]\n"
#endif
#ifdef CONFIG_BRLAPI
    "-chardev braille,id=id[,mux=on|off][,logfile=PATH][,logappend=on|off]\n"
#endif
#if defined(__linux__) || defined(__sun__) || defined(__FreeBSD__) \
        || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
    "-chardev serial,id=id,path=path[,mux=on|off][,logfile=PATH][,logappend=on|off]\n"
    "-chardev tty,id=id,path=path[,mux=on|off][,logfile=PATH][,logappend=on|off]\n"
#endif
#if defined(__linux__) || defined(__FreeBSD__) || defined(__DragonFly__)
    "-chardev parallel,id=id,path=path[,mux=on|off][,logfile=PATH][,logappend=on|off]\n"
    "-chardev parport,id=id,path=path[,mux=on|off][,logfile=PATH][,logappend=on|off]\n"
#endif
#if defined(CONFIG_SPICE)
    "-chardev spicevmc,id=id,name=name[,debug=debug][,logfile=PATH][,logappend=on|off]\n"
    "-chardev spiceport,id=id,name=name[,debug=debug][,logfile=PATH][,logappend=on|off]\n"
#endif
    , QEMU_ARCH_ALL
)

STEXI

The general form of a character device option is:
@table @option
@item -chardev @var{backend},id=@var{id}[,mux=on|off][,@var{options}]
@findex -chardev
Backend is one of:
@option{null},
@option{socket},
@option{udp},
@option{msmouse},
@option{vc},
@option{ringbuf},
@option{file},
@option{pipe},
@option{console},
@option{serial},
@option{pty},
@option{stdio},
@option{braille},
@option{tty},
@option{parallel},
@option{parport},
@option{spicevmc},
@option{spiceport}.
The specific backend will determine the applicable options.

Use @code{-chardev help} to print all available chardev backend types.

All devices must have an id, which can be any string up to 127 characters long.
It is used to uniquely identify this device in other command line directives.

A character device may be used in multiplexing mode by multiple front-ends.
Specify @option{mux=on} to enable this mode.
A multiplexer is a "1:N" device, and here the "1" end is your specified chardev
backend, and the "N" end is the various parts of QEMU that can talk to a chardev.
If you create a chardev with @option{id=myid} and @option{mux=on}, QEMU will
create a multiplexer with your specified ID, and you can then configure multiple
front ends to use that chardev ID for their input/output. Up to four different
front ends can be connected to a single multiplexed chardev. (Without
multiplexing enabled, a chardev can only be used by a single front end.)
For instance you could use this to allow a single stdio chardev to be used by
two serial ports and the QEMU monitor:

@example
-chardev stdio,mux=on,id=char0 \
-mon chardev=char0,mode=readline \
-serial chardev:char0 \
-serial chardev:char0
@end example

You can have more than one multiplexer in a system configuration; for instance
you could have a TCP port multiplexed between UART 0 and UART 1, and stdio
multiplexed between the QEMU monitor and a parallel port:

@example
-chardev stdio,mux=on,id=char0 \
-mon chardev=char0,mode=readline \
-parallel chardev:char0 \
-chardev tcp,...,mux=on,id=char1 \
-serial chardev:char1 \
-serial chardev:char1
@end example

When you're using a multiplexed character device, some escape sequences are
interpreted in the input. @xref{mux_keys, Keys in the character backend
multiplexer}.

Note that some other command line options may implicitly create multiplexed
character backends; for instance @option{-serial mon:stdio} creates a
multiplexed stdio backend connected to the serial port and the QEMU monitor,
and @option{-nographic} also multiplexes the console and the monitor to
stdio.

There is currently no support for multiplexing in the other direction
(where a single QEMU front end takes input and output from multiple chardevs).

Every backend supports the @option{logfile} option, which supplies the path
to a file to record all data transmitted via the backend. The @option{logappend}
option controls whether the log file will be truncated or appended to when
opened.

@end table

The available backends are:

@table @option
@item -chardev null,id=@var{id}
A void device. This device will not emit any data, and will drop any data it
receives. The null backend does not take any options.

@item -chardev socket,id=@var{id}[,@var{TCP options} or @var{unix options}][,server][,nowait][,telnet][,websocket][,reconnect=@var{seconds}][,tls-creds=@var{id}][,tls-authz=@var{id}]

Create a two-way stream socket, which can be either a TCP or a unix socket. A
unix socket will be created if @option{path} is specified. Behaviour is
undefined if TCP options are specified for a unix socket.

@option{server} specifies that the socket shall be a listening socket.

@option{nowait} specifies that QEMU should not block waiting for a client to
connect to a listening socket.

@option{telnet} specifies that traffic on the socket should interpret telnet
escape sequences.

@option{websocket} specifies that the socket uses WebSocket protocol for
communication.

@option{reconnect} sets the timeout for reconnecting on non-server sockets when
the remote end goes away.  qemu will delay this many seconds and then attempt
to reconnect.  Zero disables reconnecting, and is the default.

@option{tls-creds} requests enablement of the TLS protocol for encryption,
and specifies the id of the TLS credentials to use for the handshake. The
credentials must be previously created with the @option{-object tls-creds}
argument.

@option{tls-auth} provides the ID of the QAuthZ authorization object against
which the client's x509 distinguished name will be validated. This object is
only resolved at time of use, so can be deleted and recreated on the fly
while the chardev server is active. If missing, it will default to denying
access.

TCP and unix socket options are given below:

@table @option

@item TCP options: port=@var{port}[,host=@var{host}][,to=@var{to}][,ipv4][,ipv6][,nodelay]

@option{host} for a listening socket specifies the local address to be bound.
For a connecting socket species the remote host to connect to. @option{host} is
optional for listening sockets. If not specified it defaults to @code{0.0.0.0}.

@option{port} for a listening socket specifies the local port to be bound. For a
connecting socket specifies the port on the remote host to connect to.
@option{port} can be given as either a port number or a service name.
@option{port} is required.

@option{to} is only relevant to listening sockets. If it is specified, and
@option{port} cannot be bound, QEMU will attempt to bind to subsequent ports up
to and including @option{to} until it succeeds. @option{to} must be specified
as a port number.

@option{ipv4} and @option{ipv6} specify that either IPv4 or IPv6 must be used.
If neither is specified the socket may use either protocol.

@option{nodelay} disables the Nagle algorithm.

@item unix options: path=@var{path}

@option{path} specifies the local path of the unix socket. @option{path} is
required.

@end table

@item -chardev udp,id=@var{id}[,host=@var{host}],port=@var{port}[,localaddr=@var{localaddr}][,localport=@var{localport}][,ipv4][,ipv6]

Sends all traffic from the guest to a remote host over UDP.

@option{host} specifies the remote host to connect to. If not specified it
defaults to @code{localhost}.

@option{port} specifies the port on the remote host to connect to. @option{port}
is required.

@option{localaddr} specifies the local address to bind to. If not specified it
defaults to @code{0.0.0.0}.

@option{localport} specifies the local port to bind to. If not specified any
available local port will be used.

@option{ipv4} and @option{ipv6} specify that either IPv4 or IPv6 must be used.
If neither is specified the device may use either protocol.

@item -chardev msmouse,id=@var{id}

Forward QEMU's emulated msmouse events to the guest. @option{msmouse} does not
take any options.

@item -chardev vc,id=@var{id}[[,width=@var{width}][,height=@var{height}]][[,cols=@var{cols}][,rows=@var{rows}]]

Connect to a QEMU text console. @option{vc} may optionally be given a specific
size.

@option{width} and @option{height} specify the width and height respectively of
the console, in pixels.

@option{cols} and @option{rows} specify that the console be sized to fit a text
console with the given dimensions.

@item -chardev ringbuf,id=@var{id}[,size=@var{size}]

Create a ring buffer with fixed size @option{size}.
@var{size} must be a power of two and defaults to @code{64K}.

@item -chardev file,id=@var{id},path=@var{path}

Log all traffic received from the guest to a file.

@option{path} specifies the path of the file to be opened. This file will be
created if it does not already exist, and overwritten if it does. @option{path}
is required.

@item -chardev pipe,id=@var{id},path=@var{path}

Create a two-way connection to the guest. The behaviour differs slightly between
Windows hosts and other hosts:

On Windows, a single duplex pipe will be created at
@file{\\.pipe\@option{path}}.

On other hosts, 2 pipes will be created called @file{@option{path}.in} and
@file{@option{path}.out}. Data written to @file{@option{path}.in} will be
received by the guest. Data written by the guest can be read from
@file{@option{path}.out}. QEMU will not create these fifos, and requires them to
be present.

@option{path} forms part of the pipe path as described above. @option{path} is
required.

@item -chardev console,id=@var{id}

Send traffic from the guest to QEMU's standard output. @option{console} does not
take any options.

@option{console} is only available on Windows hosts.

@item -chardev serial,id=@var{id},path=@option{path}

Send traffic from the guest to a serial device on the host.

On Unix hosts serial will actually accept any tty device,
not only serial lines.

@option{path} specifies the name of the serial device to open.

@item -chardev pty,id=@var{id}

Create a new pseudo-terminal on the host and connect to it. @option{pty} does
not take any options.

@option{pty} is not available on Windows hosts.

@item -chardev stdio,id=@var{id}[,signal=on|off]
Connect to standard input and standard output of the QEMU process.

@option{signal} controls if signals are enabled on the terminal, that includes
exiting QEMU with the key sequence @key{Control-c}. This option is enabled by
default, use @option{signal=off} to disable it.

@item -chardev braille,id=@var{id}

Connect to a local BrlAPI server. @option{braille} does not take any options.

@item -chardev tty,id=@var{id},path=@var{path}

@option{tty} is only available on Linux, Sun, FreeBSD, NetBSD, OpenBSD and
DragonFlyBSD hosts.  It is an alias for @option{serial}.

@option{path} specifies the path to the tty. @option{path} is required.

@item -chardev parallel,id=@var{id},path=@var{path}
@itemx -chardev parport,id=@var{id},path=@var{path}

@option{parallel} is only available on Linux, FreeBSD and DragonFlyBSD hosts.

Connect to a local parallel port.

@option{path} specifies the path to the parallel port device. @option{path} is
required.

@item -chardev spicevmc,id=@var{id},debug=@var{debug},name=@var{name}

@option{spicevmc} is only available when spice support is built in.

@option{debug} debug level for spicevmc

@option{name} name of spice channel to connect to

Connect to a spice virtual machine channel, such as vdiport.

@item -chardev spiceport,id=@var{id},debug=@var{debug},name=@var{name}

@option{spiceport} is only available when spice support is built in.

@option{debug} debug level for spicevmc

@option{name} name of spice port to connect to

Connect to a spice port, allowing a Spice client to handle the traffic
identified by a name (preferably a fqdn).
ETEXI

STEXI
@end table
ETEXI
DEFHEADING()

DEFHEADING(Bluetooth(R) options:)
STEXI
@table @option
ETEXI

DEF("bt", HAS_ARG, QEMU_OPTION_bt, \
    "-bt hci,null    dumb bluetooth HCI - doesn't respond to commands\n" \
    "-bt hci,host[:id]\n" \
    "                use host's HCI with the given name\n" \
    "-bt hci[,vlan=n]\n" \
    "                emulate a standard HCI in virtual scatternet 'n'\n" \
    "-bt vhci[,vlan=n]\n" \
    "                add host computer to virtual scatternet 'n' using VHCI\n" \
    "-bt device:dev[,vlan=n]\n" \
    "                emulate a bluetooth device 'dev' in scatternet 'n'\n",
    QEMU_ARCH_ALL)
STEXI
@item -bt hci[...]
@findex -bt
Defines the function of the corresponding Bluetooth HCI.  -bt options
are matched with the HCIs present in the chosen machine type.  For
example when emulating a machine with only one HCI built into it, only
the first @code{-bt hci[...]} option is valid and defines the HCI's
logic.  The Transport Layer is decided by the machine type.  Currently
the machines @code{n800} and @code{n810} have one HCI and all other
machines have none.

Note: This option and the whole bluetooth subsystem is considered as deprecated.
If you still use it, please send a mail to @email{qemu-devel@@nongnu.org} where
you describe your usecase.

@anchor{bt-hcis}
The following three types are recognized:

@table @option
@item -bt hci,null
(default) The corresponding Bluetooth HCI assumes no internal logic
and will not respond to any HCI commands or emit events.

@item -bt hci,host[:@var{id}]
(@code{bluez} only) The corresponding HCI passes commands / events
to / from the physical HCI identified by the name @var{id} (default:
@code{hci0}) on the computer running QEMU.  Only available on @code{bluez}
capable systems like Linux.

@item -bt hci[,vlan=@var{n}]
Add a virtual, standard HCI that will participate in the Bluetooth
scatternet @var{n} (default @code{0}).  Similarly to @option{-net}
VLANs, devices inside a bluetooth network @var{n} can only communicate
with other devices in the same network (scatternet).
@end table

@item -bt vhci[,vlan=@var{n}]
(Linux-host only) Create a HCI in scatternet @var{n} (default 0) attached
to the host bluetooth stack instead of to the emulated target.  This
allows the host and target machines to participate in a common scatternet
and communicate.  Requires the Linux @code{vhci} driver installed.  Can
be used as following:

@example
@value{qemu_system} [...OPTIONS...] -bt hci,vlan=5 -bt vhci,vlan=5
@end example

@item -bt device:@var{dev}[,vlan=@var{n}]
Emulate a bluetooth device @var{dev} and place it in network @var{n}
(default @code{0}).  QEMU can only emulate one type of bluetooth devices
currently:

@table @option
@item keyboard
Virtual wireless keyboard implementing the HIDP bluetooth profile.
@end table
ETEXI

STEXI
@end table
ETEXI
DEFHEADING()

#ifdef CONFIG_TPM
DEFHEADING(TPM device options:)

DEF("tpmdev", HAS_ARG, QEMU_OPTION_tpmdev, \
    "-tpmdev passthrough,id=id[,path=path][,cancel-path=path]\n"
    "                use path to provide path to a character device; default is /dev/tpm0\n"
    "                use cancel-path to provide path to TPM's cancel sysfs entry; if\n"
    "                not provided it will be searched for in /sys/class/misc/tpm?/device\n"
    "-tpmdev emulator,id=id,chardev=dev\n"
    "                configure the TPM device using chardev backend\n",
    QEMU_ARCH_ALL)
STEXI

The general form of a TPM device option is:
@table @option

@item -tpmdev @var{backend},id=@var{id}[,@var{options}]
@findex -tpmdev

The specific backend type will determine the applicable options.
The @code{-tpmdev} option creates the TPM backend and requires a
@code{-device} option that specifies the TPM frontend interface model.

Use @code{-tpmdev help} to print all available TPM backend types.

@end table

The available backends are:

@table @option

@item -tpmdev passthrough,id=@var{id},path=@var{path},cancel-path=@var{cancel-path}

(Linux-host only) Enable access to the host's TPM using the passthrough
driver.

@option{path} specifies the path to the host's TPM device, i.e., on
a Linux host this would be @code{/dev/tpm0}.
@option{path} is optional and by default @code{/dev/tpm0} is used.

@option{cancel-path} specifies the path to the host TPM device's sysfs
entry allowing for cancellation of an ongoing TPM command.
@option{cancel-path} is optional and by default QEMU will search for the
sysfs entry to use.

Some notes about using the host's TPM with the passthrough driver:

The TPM device accessed by the passthrough driver must not be
used by any other application on the host.

Since the host's firmware (BIOS/UEFI) has already initialized the TPM,
the VM's firmware (BIOS/UEFI) will not be able to initialize the
TPM again and may therefore not show a TPM-specific menu that would
otherwise allow the user to configure the TPM, e.g., allow the user to
enable/disable or activate/deactivate the TPM.
Further, if TPM ownership is released from within a VM then the host's TPM
will get disabled and deactivated. To enable and activate the
TPM again afterwards, the host has to be rebooted and the user is
required to enter the firmware's menu to enable and activate the TPM.
If the TPM is left disabled and/or deactivated most TPM commands will fail.

To create a passthrough TPM use the following two options:
@example
-tpmdev passthrough,id=tpm0 -device tpm-tis,tpmdev=tpm0
@end example
Note that the @code{-tpmdev} id is @code{tpm0} and is referenced by
@code{tpmdev=tpm0} in the device option.

@item -tpmdev emulator,id=@var{id},chardev=@var{dev}

(Linux-host only) Enable access to a TPM emulator using Unix domain socket based
chardev backend.

@option{chardev} specifies the unique ID of a character device backend that provides connection to the software TPM server.

To create a TPM emulator backend device with chardev socket backend:
@example

-chardev socket,id=chrtpm,path=/tmp/swtpm-sock -tpmdev emulator,id=tpm0,chardev=chrtpm -device tpm-tis,tpmdev=tpm0

@end example

ETEXI

STEXI
@end table
ETEXI
DEFHEADING()

#endif

DEFHEADING(Linux/Multiboot boot specific:)
STEXI

When using these options, you can use a given Linux or Multiboot
kernel without installing it in the disk image. It can be useful
for easier testing of various kernels.

@table @option
ETEXI

DEF("kernel", HAS_ARG, QEMU_OPTION_kernel, \
    "-kernel bzImage use 'bzImage' as kernel image\n", QEMU_ARCH_ALL)
STEXI
@item -kernel @var{bzImage}
@findex -kernel
Use @var{bzImage} as kernel image. The kernel can be either a Linux kernel
or in multiboot format.
ETEXI

DEF("append", HAS_ARG, QEMU_OPTION_append, \
    "-append cmdline use 'cmdline' as kernel command line\n", QEMU_ARCH_ALL)
STEXI
@item -append @var{cmdline}
@findex -append
Use @var{cmdline} as kernel command line
ETEXI

DEF("initrd", HAS_ARG, QEMU_OPTION_initrd, \
           "-initrd file    use 'file' as initial ram disk\n", QEMU_ARCH_ALL)
STEXI
@item -initrd @var{file}
@findex -initrd
Use @var{file} as initial ram disk.

@item -initrd "@var{file1} arg=foo,@var{file2}"

This syntax is only available with multiboot.

Use @var{file1} and @var{file2} as modules and pass arg=foo as parameter to the
first module.
ETEXI

DEF("dtb", HAS_ARG, QEMU_OPTION_dtb, \
    "-dtb    file    use 'file' as device tree image\n", QEMU_ARCH_ALL)
STEXI
@item -dtb @var{file}
@findex -dtb
Use @var{file} as a device tree binary (dtb) image and pass it to the kernel
on boot.
ETEXI

STEXI
@end table
ETEXI
DEFHEADING()

DEFHEADING(Debug/Expert options:)
STEXI
@table @option
ETEXI

DEF("fw_cfg", HAS_ARG, QEMU_OPTION_fwcfg,
    "-fw_cfg [name=]<name>,file=<file>\n"
    "                add named fw_cfg entry with contents from file\n"
    "-fw_cfg [name=]<name>,string=<str>\n"
    "                add named fw_cfg entry with contents from string\n",
    QEMU_ARCH_ALL)
STEXI

@item -fw_cfg [name=]@var{name},file=@var{file}
@findex -fw_cfg
Add named fw_cfg entry with contents from file @var{file}.

@item -fw_cfg [name=]@var{name},string=@var{str}
Add named fw_cfg entry with contents from string @var{str}.

The terminating NUL character of the contents of @var{str} will not be
included as part of the fw_cfg item data. To insert contents with
embedded NUL characters, you have to use the @var{file} parameter.

The fw_cfg entries are passed by QEMU through to the guest.

Example:
@example
    -fw_cfg name=opt/com.mycompany/blob,file=./my_blob.bin
@end example
creates an fw_cfg entry named opt/com.mycompany/blob with contents
from ./my_blob.bin.

ETEXI

DEF("serial", HAS_ARG, QEMU_OPTION_serial, \
    "-serial dev     redirect the serial port to char device 'dev'\n",
    QEMU_ARCH_ALL)
STEXI
@item -serial @var{dev}
@findex -serial
Redirect the virtual serial port to host character device
@var{dev}. The default device is @code{vc} in graphical mode and
@code{stdio} in non graphical mode.

This option can be used several times to simulate up to 4 serial
ports.

Use @code{-serial none} to disable all serial ports.

Available character devices are:
@table @option
@item vc[:@var{W}x@var{H}]
Virtual console. Optionally, a width and height can be given in pixel with
@example
vc:800x600
@end example
It is also possible to specify width or height in characters:
@example
vc:80Cx24C
@end example
@item pty
[Linux only] Pseudo TTY (a new PTY is automatically allocated)
@item none
No device is allocated.
@item null
void device
@item chardev:@var{id}
Use a named character device defined with the @code{-chardev} option.
@item /dev/XXX
[Linux only] Use host tty, e.g. @file{/dev/ttyS0}. The host serial port
parameters are set according to the emulated ones.
@item /dev/parport@var{N}
[Linux only, parallel port only] Use host parallel port
@var{N}. Currently SPP and EPP parallel port features can be used.
@item file:@var{filename}
Write output to @var{filename}. No character can be read.
@item stdio
[Unix only] standard input/output
@item pipe:@var{filename}
name pipe @var{filename}
@item COM@var{n}
[Windows only] Use host serial port @var{n}
@item udp:[@var{remote_host}]:@var{remote_port}[@@[@var{src_ip}]:@var{src_port}]
This implements UDP Net Console.
When @var{remote_host} or @var{src_ip} are not specified
they default to @code{0.0.0.0}.
When not using a specified @var{src_port} a random port is automatically chosen.

If you just want a simple readonly console you can use @code{netcat} or
@code{nc}, by starting QEMU with: @code{-serial udp::4555} and nc as:
@code{nc -u -l -p 4555}. Any time QEMU writes something to that port it
will appear in the netconsole session.

If you plan to send characters back via netconsole or you want to stop
and start QEMU a lot of times, you should have QEMU use the same
source port each time by using something like @code{-serial
udp::4555@@:4556} to QEMU. Another approach is to use a patched
version of netcat which can listen to a TCP port and send and receive
characters via udp.  If you have a patched version of netcat which
activates telnet remote echo and single char transfer, then you can
use the following options to set up a netcat redirector to allow
telnet on port 5555 to access the QEMU port.
@table @code
@item QEMU Options:
-serial udp::4555@@:4556
@item netcat options:
-u -P 4555 -L 0.0.0.0:4556 -t -p 5555 -I -T
@item telnet options:
localhost 5555
@end table

@item tcp:[@var{host}]:@var{port}[,@var{server}][,nowait][,nodelay][,reconnect=@var{seconds}]
The TCP Net Console has two modes of operation.  It can send the serial
I/O to a location or wait for a connection from a location.  By default
the TCP Net Console is sent to @var{host} at the @var{port}.  If you use
the @var{server} option QEMU will wait for a client socket application
to connect to the port before continuing, unless the @code{nowait}
option was specified.  The @code{nodelay} option disables the Nagle buffering
algorithm.  The @code{reconnect} option only applies if @var{noserver} is
set, if the connection goes down it will attempt to reconnect at the
given interval.  If @var{host} is omitted, 0.0.0.0 is assumed. Only
one TCP connection at a time is accepted. You can use @code{telnet} to
connect to the corresponding character device.
@table @code
@item Example to send tcp console to 192.168.0.2 port 4444
-serial tcp:192.168.0.2:4444
@item Example to listen and wait on port 4444 for connection
-serial tcp::4444,server
@item Example to not wait and listen on ip 192.168.0.100 port 4444
-serial tcp:192.168.0.100:4444,server,nowait
@end table

@item telnet:@var{host}:@var{port}[,server][,nowait][,nodelay]
The telnet protocol is used instead of raw tcp sockets.  The options
work the same as if you had specified @code{-serial tcp}.  The
difference is that the port acts like a telnet server or client using
telnet option negotiation.  This will also allow you to send the
MAGIC_SYSRQ sequence if you use a telnet that supports sending the break
sequence.  Typically in unix telnet you do it with Control-] and then
type "send break" followed by pressing the enter key.

@item websocket:@var{host}:@var{port},server[,nowait][,nodelay]
The WebSocket protocol is used instead of raw tcp socket. The port acts as
a WebSocket server. Client mode is not supported.

@item unix:@var{path}[,server][,nowait][,reconnect=@var{seconds}]
A unix domain socket is used instead of a tcp socket.  The option works the
same as if you had specified @code{-serial tcp} except the unix domain socket
@var{path} is used for connections.

@item mon:@var{dev_string}
This is a special option to allow the monitor to be multiplexed onto
another serial port.  The monitor is accessed with key sequence of
@key{Control-a} and then pressing @key{c}.
@var{dev_string} should be any one of the serial devices specified
above.  An example to multiplex the monitor onto a telnet server
listening on port 4444 would be:
@table @code
@item -serial mon:telnet::4444,server,nowait
@end table
When the monitor is multiplexed to stdio in this way, Ctrl+C will not terminate
QEMU any more but will be passed to the guest instead.

@item braille
Braille device.  This will use BrlAPI to display the braille output on a real
or fake device.

@item msmouse
Three button serial mouse. Configure the guest to use Microsoft protocol.
@end table
ETEXI

DEF("parallel", HAS_ARG, QEMU_OPTION_parallel, \
    "-parallel dev   redirect the parallel port to char device 'dev'\n",
    QEMU_ARCH_ALL)
STEXI
@item -parallel @var{dev}
@findex -parallel
Redirect the virtual parallel port to host device @var{dev} (same
devices as the serial port). On Linux hosts, @file{/dev/parportN} can
be used to use hardware devices connected on the corresponding host
parallel port.

This option can be used several times to simulate up to 3 parallel
ports.

Use @code{-parallel none} to disable all parallel ports.
ETEXI

DEF("monitor", HAS_ARG, QEMU_OPTION_monitor, \
    "-monitor dev    redirect the monitor to char device 'dev'\n",
    QEMU_ARCH_ALL)
STEXI
@item -monitor @var{dev}
@findex -monitor
Redirect the monitor to host device @var{dev} (same devices as the
serial port).
The default device is @code{vc} in graphical mode and @code{stdio} in
non graphical mode.
Use @code{-monitor none} to disable the default monitor.
ETEXI
DEF("qmp", HAS_ARG, QEMU_OPTION_qmp, \
    "-qmp dev        like -monitor but opens in 'control' mode\n",
    QEMU_ARCH_ALL)
STEXI
@item -qmp @var{dev}
@findex -qmp
Like -monitor but opens in 'control' mode.
ETEXI
DEF("qmp-pretty", HAS_ARG, QEMU_OPTION_qmp_pretty, \
    "-qmp-pretty dev like -qmp but uses pretty JSON formatting\n",
    QEMU_ARCH_ALL)
STEXI
@item -qmp-pretty @var{dev}
@findex -qmp-pretty
Like -qmp but uses pretty JSON formatting.
ETEXI

DEF("mon", HAS_ARG, QEMU_OPTION_mon, \
    "-mon [chardev=]name[,mode=readline|control][,pretty[=on|off]]\n", QEMU_ARCH_ALL)
STEXI
@item -mon [chardev=]name[,mode=readline|control][,pretty[=on|off]]
@findex -mon
Setup monitor on chardev @var{name}. @code{pretty} turns on JSON pretty printing
easing human reading and debugging.
ETEXI

DEF("debugcon", HAS_ARG, QEMU_OPTION_debugcon, \
    "-debugcon dev   redirect the debug console to char device 'dev'\n",
    QEMU_ARCH_ALL)
STEXI
@item -debugcon @var{dev}
@findex -debugcon
Redirect the debug console to host device @var{dev} (same devices as the
serial port).  The debug console is an I/O port which is typically port
0xe9; writing to that I/O port sends output to this device.
The default device is @code{vc} in graphical mode and @code{stdio} in
non graphical mode.
ETEXI

DEF("pidfile", HAS_ARG, QEMU_OPTION_pidfile, \
    "-pidfile file   write PID to 'file'\n", QEMU_ARCH_ALL)
STEXI
@item -pidfile @var{file}
@findex -pidfile
Store the QEMU process PID in @var{file}. It is useful if you launch QEMU
from a script.
ETEXI

DEF("singlestep", 0, QEMU_OPTION_singlestep, \
    "-singlestep     always run in singlestep mode\n", QEMU_ARCH_ALL)
STEXI
@item -singlestep
@findex -singlestep
Run the emulation in single step mode.
ETEXI

DEF("preconfig", 0, QEMU_OPTION_preconfig, \
    "--preconfig     pause QEMU before machine is initialized (experimental)\n",
    QEMU_ARCH_ALL)
STEXI
@item --preconfig
@findex --preconfig
Pause QEMU for interactive configuration before the machine is created,
which allows querying and configuring properties that will affect
machine initialization.  Use QMP command 'x-exit-preconfig' to exit
the preconfig state and move to the next state (i.e. run guest if -S
isn't used or pause the second time if -S is used).  This option is
experimental.
ETEXI

DEF("S", 0, QEMU_OPTION_S, \
    "-S              freeze CPU at startup (use 'c' to start execution)\n",
    QEMU_ARCH_ALL)
STEXI
@item -S
@findex -S
Do not start CPU at startup (you must type 'c' in the monitor).
ETEXI

DEF("realtime", HAS_ARG, QEMU_OPTION_realtime,
    "-realtime [mlock=on|off]\n"
    "                run qemu with realtime features\n"
    "                mlock=on|off controls mlock support (default: on)\n",
    QEMU_ARCH_ALL)
STEXI
@item -realtime mlock=on|off
@findex -realtime
Run qemu with realtime features.
mlocking qemu and guest memory can be enabled via @option{mlock=on}
(enabled by default).
ETEXI

DEF("overcommit", HAS_ARG, QEMU_OPTION_overcommit,
    "-overcommit [mem-lock=on|off][cpu-pm=on|off]\n"
    "                run qemu with overcommit hints\n"
    "                mem-lock=on|off controls memory lock support (default: off)\n"
    "                cpu-pm=on|off controls cpu power management (default: off)\n",
    QEMU_ARCH_ALL)
STEXI
@item -overcommit mem-lock=on|off
@item -overcommit cpu-pm=on|off
@findex -overcommit
Run qemu with hints about host resource overcommit. The default is
to assume that host overcommits all resources.

Locking qemu and guest memory can be enabled via @option{mem-lock=on} (disabled
by default).  This works when host memory is not overcommitted and reduces the
worst-case latency for guest.  This is equivalent to @option{realtime}.

Guest ability to manage power state of host cpus (increasing latency for other
processes on the same host cpu, but decreasing latency for guest) can be
enabled via @option{cpu-pm=on} (disabled by default).  This works best when
host CPU is not overcommitted. When used, host estimates of CPU cycle and power
utilization will be incorrect, not taking into account guest idle time.
ETEXI

DEF("gdb", HAS_ARG, QEMU_OPTION_gdb, \
    "-gdb dev        wait for gdb connection on 'dev'\n", QEMU_ARCH_ALL)
STEXI
@item -gdb @var{dev}
@findex -gdb
Wait for gdb connection on device @var{dev} (@pxref{gdb_usage}). Typical
connections will likely be TCP-based, but also UDP, pseudo TTY, or even
stdio are reasonable use case. The latter is allowing to start QEMU from
within gdb and establish the connection via a pipe:
@example
(gdb) target remote | exec @value{qemu_system} -gdb stdio ...
@end example
ETEXI

DEF("s", 0, QEMU_OPTION_s, \
    "-s              shorthand for -gdb tcp::" DEFAULT_GDBSTUB_PORT "\n",
    QEMU_ARCH_ALL)
STEXI
@item -s
@findex -s
Shorthand for -gdb tcp::1234, i.e. open a gdbserver on TCP port 1234
(@pxref{gdb_usage}).
ETEXI

DEF("d", HAS_ARG, QEMU_OPTION_d, \
    "-d item1,...    enable logging of specified items (use '-d help' for a list of log items)\n",
    QEMU_ARCH_ALL)
STEXI
@item -d @var{item1}[,...]
@findex -d
Enable logging of specified items. Use '-d help' for a list of log items.
ETEXI

DEF("D", HAS_ARG, QEMU_OPTION_D, \
    "-D logfile      output log to logfile (default stderr)\n",
    QEMU_ARCH_ALL)
STEXI
@item -D @var{logfile}
@findex -D
Output log in @var{logfile} instead of to stderr
ETEXI

DEF("dfilter", HAS_ARG, QEMU_OPTION_DFILTER, \
    "-dfilter range,..  filter debug output to range of addresses (useful for -d cpu,exec,etc..)\n",
    QEMU_ARCH_ALL)
STEXI
@item -dfilter @var{range1}[,...]
@findex -dfilter
Filter debug output to that relevant to a range of target addresses. The filter
spec can be either @var{start}+@var{size}, @var{start}-@var{size} or
@var{start}..@var{end} where @var{start} @var{end} and @var{size} are the
addresses and sizes required. For example:
@example
    -dfilter 0x8000..0x8fff,0xffffffc000080000+0x200,0xffffffc000060000-0x1000
@end example
Will dump output for any code in the 0x1000 sized block starting at 0x8000 and
the 0x200 sized block starting at 0xffffffc000080000 and another 0x1000 sized
block starting at 0xffffffc00005f000.
ETEXI

DEF("seed", HAS_ARG, QEMU_OPTION_seed, \
    "-seed number       seed the pseudo-random number generator\n",
    QEMU_ARCH_ALL)
STEXI
@item -seed @var{number}
@findex -seed
Force the guest to use a deterministic pseudo-random number generator, seeded
with @var{number}.  This does not affect crypto routines within the host.
ETEXI

DEF("L", HAS_ARG, QEMU_OPTION_L, \
    "-L path         set the directory for the BIOS, VGA BIOS and keymaps\n",
    QEMU_ARCH_ALL)
STEXI
@item -L  @var{path}
@findex -L
Set the directory for the BIOS, VGA BIOS and keymaps.

To list all the data directories, use @code{-L help}.
ETEXI

DEF("bios", HAS_ARG, QEMU_OPTION_bios, \
    "-bios file      set the filename for the BIOS\n", QEMU_ARCH_ALL)
STEXI
@item -bios @var{file}
@findex -bios
Set the filename for the BIOS.
ETEXI

DEF("enable-kvm", 0, QEMU_OPTION_enable_kvm, \
    "-enable-kvm     enable KVM full virtualization support\n", QEMU_ARCH_ALL)
STEXI
@item -enable-kvm
@findex -enable-kvm
Enable KVM full virtualization support. This option is only available
if KVM support is enabled when compiling.
ETEXI

DEF("xen-domid", HAS_ARG, QEMU_OPTION_xen_domid,
    "-xen-domid id   specify xen guest domain id\n", QEMU_ARCH_ALL)
DEF("xen-attach", 0, QEMU_OPTION_xen_attach,
    "-xen-attach     attach to existing xen domain\n"
    "                libxl will use this when starting QEMU\n",
    QEMU_ARCH_ALL)
DEF("xen-domid-restrict", 0, QEMU_OPTION_xen_domid_restrict,
    "-xen-domid-restrict     restrict set of available xen operations\n"
    "                        to specified domain id. (Does not affect\n"
    "                        xenpv machine type).\n",
    QEMU_ARCH_ALL)
STEXI
@item -xen-domid @var{id}
@findex -xen-domid
Specify xen guest domain @var{id} (XEN only).
@item -xen-attach
@findex -xen-attach
Attach to existing xen domain.
libxl will use this when starting QEMU (XEN only).
@findex -xen-domid-restrict
Restrict set of available xen operations to specified domain id (XEN only).
ETEXI

DEF("no-reboot", 0, QEMU_OPTION_no_reboot, \
    "-no-reboot      exit instead of rebooting\n", QEMU_ARCH_ALL)
STEXI
@item -no-reboot
@findex -no-reboot
Exit instead of rebooting.
ETEXI

DEF("no-shutdown", 0, QEMU_OPTION_no_shutdown, \
    "-no-shutdown    stop before shutdown\n", QEMU_ARCH_ALL)
STEXI
@item -no-shutdown
@findex -no-shutdown
Don't exit QEMU on guest shutdown, but instead only stop the emulation.
This allows for instance switching to monitor to commit changes to the
disk image.
ETEXI

DEF("loadvm", HAS_ARG, QEMU_OPTION_loadvm, \
    "-loadvm [tag|id]\n" \
    "                start right away with a saved state (loadvm in monitor)\n",
    QEMU_ARCH_ALL)
STEXI
@item -loadvm @var{file}
@findex -loadvm
Start right away with a saved state (@code{loadvm} in monitor)
ETEXI

#ifndef _WIN32
DEF("daemonize", 0, QEMU_OPTION_daemonize, \
    "-daemonize      daemonize QEMU after initializing\n", QEMU_ARCH_ALL)
#endif
STEXI
@item -daemonize
@findex -daemonize
Daemonize the QEMU process after initialization.  QEMU will not detach from
standard IO until it is ready to receive connections on any of its devices.
This option is a useful way for external programs to launch QEMU without having
to cope with initialization race conditions.
ETEXI

DEF("option-rom", HAS_ARG, QEMU_OPTION_option_rom, \
    "-option-rom rom load a file, rom, into the option ROM space\n",
    QEMU_ARCH_ALL)
STEXI
@item -option-rom @var{file}
@findex -option-rom
Load the contents of @var{file} as an option ROM.
This option is useful to load things like EtherBoot.
ETEXI

DEF("rtc", HAS_ARG, QEMU_OPTION_rtc, \
    "-rtc [base=utc|localtime|<datetime>][,clock=host|rt|vm][,driftfix=none|slew]\n" \
    "                set the RTC base and clock, enable drift fix for clock ticks (x86 only)\n",
    QEMU_ARCH_ALL)

STEXI

@item -rtc [base=utc|localtime|@var{datetime}][,clock=host|rt|vm][,driftfix=none|slew]
@findex -rtc
Specify @option{base} as @code{utc} or @code{localtime} to let the RTC start at the current
UTC or local time, respectively. @code{localtime} is required for correct date in
MS-DOS or Windows. To start at a specific point in time, provide @var{datetime} in the
format @code{2006-06-17T16:01:21} or @code{2006-06-17}. The default base is UTC.

By default the RTC is driven by the host system time. This allows using of the
RTC as accurate reference clock inside the guest, specifically if the host
time is smoothly following an accurate external reference clock, e.g. via NTP.
If you want to isolate the guest time from the host, you can set @option{clock}
to @code{rt} instead, which provides a host monotonic clock if host support it.
To even prevent the RTC from progressing during suspension, you can set @option{clock}
to @code{vm} (virtual clock). @samp{clock=vm} is recommended especially in
icount mode in order to preserve determinism; however, note that in icount mode
the speed of the virtual clock is variable and can in general differ from the
host clock.

Enable @option{driftfix} (i386 targets only) if you experience time drift problems,
specifically with Windows' ACPI HAL. This option will try to figure out how
many timer interrupts were not processed by the Windows guest and will
re-inject them.
ETEXI

DEF("icount", HAS_ARG, QEMU_OPTION_icount, \
    "-icount [shift=N|auto][,align=on|off][,sleep=on|off,rr=record|replay,rrfile=<filename>,rrsnapshot=<snapshot>]\n" \
    "                enable virtual instruction counter with 2^N clock ticks per\n" \
    "                instruction, enable aligning the host and virtual clocks\n" \
    "                or disable real time cpu sleeping\n", QEMU_ARCH_ALL)
STEXI
@item -icount [shift=@var{N}|auto][,rr=record|replay,rrfile=@var{filename},rrsnapshot=@var{snapshot}]
@findex -icount
Enable virtual instruction counter.  The virtual cpu will execute one
instruction every 2^@var{N} ns of virtual time.  If @code{auto} is specified
then the virtual cpu speed will be automatically adjusted to keep virtual
time within a few seconds of real time.

When the virtual cpu is sleeping, the virtual time will advance at default
speed unless @option{sleep=on|off} is specified.
With @option{sleep=on|off}, the virtual time will jump to the next timer deadline
instantly whenever the virtual cpu goes to sleep mode and will not advance
if no timer is enabled. This behavior give deterministic execution times from
the guest point of view.

Note that while this option can give deterministic behavior, it does not
provide cycle accurate emulation.  Modern CPUs contain superscalar out of
order cores with complex cache hierarchies.  The number of instructions
executed often has little or no correlation with actual performance.

@option{align=on} will activate the delay algorithm which will try
to synchronise the host clock and the virtual clock. The goal is to
have a guest running at the real frequency imposed by the shift option.
Whenever the guest clock is behind the host clock and if
@option{align=on} is specified then we print a message to the user
to inform about the delay.
Currently this option does not work when @option{shift} is @code{auto}.
Note: The sync algorithm will work for those shift values for which
the guest clock runs ahead of the host clock. Typically this happens
when the shift value is high (how high depends on the host machine).

When @option{rr} option is specified deterministic record/replay is enabled.
Replay log is written into @var{filename} file in record mode and
read from this file in replay mode.

Option rrsnapshot is used to create new vm snapshot named @var{snapshot}
at the start of execution recording. In replay mode this option is used
to load the initial VM state.
ETEXI

DEF("watchdog", HAS_ARG, QEMU_OPTION_watchdog, \
    "-watchdog model\n" \
    "                enable virtual hardware watchdog [default=none]\n",
    QEMU_ARCH_ALL)
STEXI
@item -watchdog @var{model}
@findex -watchdog
Create a virtual hardware watchdog device.  Once enabled (by a guest
action), the watchdog must be periodically polled by an agent inside
the guest or else the guest will be restarted. Choose a model for
which your guest has drivers.

The @var{model} is the model of hardware watchdog to emulate. Use
@code{-watchdog help} to list available hardware models. Only one
watchdog can be enabled for a guest.

The following models may be available:
@table @option
@item ib700
iBASE 700 is a very simple ISA watchdog with a single timer.
@item i6300esb
Intel 6300ESB I/O controller hub is a much more featureful PCI-based
dual-timer watchdog.
@item diag288
A virtual watchdog for s390x backed by the diagnose 288 hypercall
(currently KVM only).
@end table
ETEXI

DEF("watchdog-action", HAS_ARG, QEMU_OPTION_watchdog_action, \
    "-watchdog-action reset|shutdown|poweroff|inject-nmi|pause|debug|none\n" \
    "                action when watchdog fires [default=reset]\n",
    QEMU_ARCH_ALL)
STEXI
@item -watchdog-action @var{action}
@findex -watchdog-action

The @var{action} controls what QEMU will do when the watchdog timer
expires.
The default is
@code{reset} (forcefully reset the guest).
Other possible actions are:
@code{shutdown} (attempt to gracefully shutdown the guest),
@code{poweroff} (forcefully poweroff the guest),
@code{inject-nmi} (inject a NMI into the guest),
@code{pause} (pause the guest),
@code{debug} (print a debug message and continue), or
@code{none} (do nothing).

Note that the @code{shutdown} action requires that the guest responds
to ACPI signals, which it may not be able to do in the sort of
situations where the watchdog would have expired, and thus
@code{-watchdog-action shutdown} is not recommended for production use.

Examples:

@table @code
@item -watchdog i6300esb -watchdog-action pause
@itemx -watchdog ib700
@end table
ETEXI

DEF("echr", HAS_ARG, QEMU_OPTION_echr, \
    "-echr chr       set terminal escape character instead of ctrl-a\n",
    QEMU_ARCH_ALL)
STEXI

@item -echr @var{numeric_ascii_value}
@findex -echr
Change the escape character used for switching to the monitor when using
monitor and serial sharing.  The default is @code{0x01} when using the
@code{-nographic} option.  @code{0x01} is equal to pressing
@code{Control-a}.  You can select a different character from the ascii
control keys where 1 through 26 map to Control-a through Control-z.  For
instance you could use the either of the following to change the escape
character to Control-t.
@table @code
@item -echr 0x14
@itemx -echr 20
@end table
ETEXI

DEF("show-cursor", 0, QEMU_OPTION_show_cursor, \
    "-show-cursor    show cursor\n", QEMU_ARCH_ALL)
STEXI
@item -show-cursor
@findex -show-cursor
Show cursor.
ETEXI

DEF("tb-size", HAS_ARG, QEMU_OPTION_tb_size, \
    "-tb-size n      set TB size\n", QEMU_ARCH_ALL)
STEXI
@item -tb-size @var{n}
@findex -tb-size
Set TB size.
ETEXI

DEF("incoming", HAS_ARG, QEMU_OPTION_incoming, \
    "-incoming tcp:[host]:port[,to=maxport][,ipv4][,ipv6]\n" \
    "-incoming rdma:host:port[,ipv4][,ipv6]\n" \
    "-incoming unix:socketpath\n" \
    "                prepare for incoming migration, listen on\n" \
    "                specified protocol and socket address\n" \
    "-incoming fd:fd\n" \
    "-incoming exec:cmdline\n" \
    "                accept incoming migration on given file descriptor\n" \
    "                or from given external command\n" \
    "-incoming defer\n" \
    "                wait for the URI to be specified via migrate_incoming\n",
    QEMU_ARCH_ALL)
STEXI
@item -incoming tcp:[@var{host}]:@var{port}[,to=@var{maxport}][,ipv4][,ipv6]
@itemx -incoming rdma:@var{host}:@var{port}[,ipv4][,ipv6]
@findex -incoming
Prepare for incoming migration, listen on a given tcp port.

@item -incoming unix:@var{socketpath}
Prepare for incoming migration, listen on a given unix socket.

@item -incoming fd:@var{fd}
Accept incoming migration from a given filedescriptor.

@item -incoming exec:@var{cmdline}
Accept incoming migration as an output from specified external command.

@item -incoming defer
Wait for the URI to be specified via migrate_incoming.  The monitor can
be used to change settings (such as migration parameters) prior to issuing
the migrate_incoming to allow the migration to begin.
ETEXI

DEF("only-migratable", 0, QEMU_OPTION_only_migratable, \
    "-only-migratable     allow only migratable devices\n", QEMU_ARCH_ALL)
STEXI
@item -only-migratable
@findex -only-migratable
Only allow migratable devices. Devices will not be allowed to enter an
unmigratable state.
ETEXI

DEF("nodefaults", 0, QEMU_OPTION_nodefaults, \
    "-nodefaults     don't create default devices\n", QEMU_ARCH_ALL)
STEXI
@item -nodefaults
@findex -nodefaults
Don't create default devices. Normally, QEMU sets the default devices like serial
port, parallel port, virtual console, monitor device, VGA adapter, floppy and
CD-ROM drive and others. The @code{-nodefaults} option will disable all those
default devices.
ETEXI

#ifndef _WIN32
DEF("chroot", HAS_ARG, QEMU_OPTION_chroot, \
    "-chroot dir     chroot to dir just before starting the VM\n",
    QEMU_ARCH_ALL)
#endif
STEXI
@item -chroot @var{dir}
@findex -chroot
Immediately before starting guest execution, chroot to the specified
directory.  Especially useful in combination with -runas.
ETEXI

#ifndef _WIN32
DEF("runas", HAS_ARG, QEMU_OPTION_runas, \
    "-runas user     change to user id user just before starting the VM\n" \
    "                user can be numeric uid:gid instead\n",
    QEMU_ARCH_ALL)
#endif
STEXI
@item -runas @var{user}
@findex -runas
Immediately before starting guest execution, drop root privileges, switching
to the specified user.
ETEXI

DEF("prom-env", HAS_ARG, QEMU_OPTION_prom_env,
    "-prom-env variable=value\n"
    "                set OpenBIOS nvram variables\n",
    QEMU_ARCH_PPC | QEMU_ARCH_SPARC)
STEXI
@item -prom-env @var{variable}=@var{value}
@findex -prom-env
Set OpenBIOS nvram @var{variable} to given @var{value} (PPC, SPARC only).
ETEXI
DEF("semihosting", 0, QEMU_OPTION_semihosting,
    "-semihosting    semihosting mode\n",
    QEMU_ARCH_ARM | QEMU_ARCH_M68K | QEMU_ARCH_XTENSA | QEMU_ARCH_LM32 |
    QEMU_ARCH_MIPS | QEMU_ARCH_NIOS2)
STEXI
@item -semihosting
@findex -semihosting
Enable semihosting mode (ARM, M68K, Xtensa, MIPS, Nios II only).
ETEXI
DEF("semihosting-config", HAS_ARG, QEMU_OPTION_semihosting_config,
    "-semihosting-config [enable=on|off][,target=native|gdb|auto][,chardev=id][,arg=str[,...]]\n" \
    "                semihosting configuration\n",
QEMU_ARCH_ARM | QEMU_ARCH_M68K | QEMU_ARCH_XTENSA | QEMU_ARCH_LM32 |
QEMU_ARCH_MIPS | QEMU_ARCH_NIOS2)
STEXI
@item -semihosting-config [enable=on|off][,target=native|gdb|auto][,chardev=id][,arg=str[,...]]
@findex -semihosting-config
Enable and configure semihosting (ARM, M68K, Xtensa, MIPS, Nios II only).
@table @option
@item target=@code{native|gdb|auto}
Defines where the semihosting calls will be addressed, to QEMU (@code{native})
or to GDB (@code{gdb}). The default is @code{auto}, which means @code{gdb}
during debug sessions and @code{native} otherwise.
@item chardev=@var{str1}
Send the output to a chardev backend output for native or auto output when not in gdb
@item arg=@var{str1},arg=@var{str2},...
Allows the user to pass input arguments, and can be used multiple times to build
up a list. The old-style @code{-kernel}/@code{-append} method of passing a
command line is still supported for backward compatibility. If both the
@code{--semihosting-config arg} and the @code{-kernel}/@code{-append} are
specified, the former is passed to semihosting as it always takes precedence.
@end table
ETEXI
DEF("old-param", 0, QEMU_OPTION_old_param,
    "-old-param      old param mode\n", QEMU_ARCH_ARM)
STEXI
@item -old-param
@findex -old-param (ARM)
Old param mode (ARM only).
ETEXI

DEF("sandbox", HAS_ARG, QEMU_OPTION_sandbox, \
    "-sandbox on[,obsolete=allow|deny][,elevateprivileges=allow|deny|children]\n" \
    "          [,spawn=allow|deny][,resourcecontrol=allow|deny]\n" \
    "                Enable seccomp mode 2 system call filter (default 'off').\n" \
    "                use 'obsolete' to allow obsolete system calls that are provided\n" \
    "                    by the kernel, but typically no longer used by modern\n" \
    "                    C library implementations.\n" \
    "                use 'elevateprivileges' to allow or deny QEMU process to elevate\n" \
    "                    its privileges by blacklisting all set*uid|gid system calls.\n" \
    "                    The value 'children' will deny set*uid|gid system calls for\n" \
    "                    main QEMU process but will allow forks and execves to run unprivileged\n" \
    "                use 'spawn' to avoid QEMU to spawn new threads or processes by\n" \
    "                     blacklisting *fork and execve\n" \
    "                use 'resourcecontrol' to disable process affinity and schedular priority\n",
    QEMU_ARCH_ALL)
STEXI
@item -sandbox @var{arg}[,obsolete=@var{string}][,elevateprivileges=@var{string}][,spawn=@var{string}][,resourcecontrol=@var{string}]
@findex -sandbox
Enable Seccomp mode 2 system call filter. 'on' will enable syscall filtering and 'off' will
disable it.  The default is 'off'.
@table @option
@item obsolete=@var{string}
Enable Obsolete system calls
@item elevateprivileges=@var{string}
Disable set*uid|gid system calls
@item spawn=@var{string}
Disable *fork and execve
@item resourcecontrol=@var{string}
Disable process affinity and schedular priority
@end table
ETEXI

DEF("readconfig", HAS_ARG, QEMU_OPTION_readconfig,
    "-readconfig <file>\n", QEMU_ARCH_ALL)
STEXI
@item -readconfig @var{file}
@findex -readconfig
Read device configuration from @var{file}. This approach is useful when you want to spawn
QEMU process with many command line options but you don't want to exceed the command line
character limit.
ETEXI
DEF("writeconfig", HAS_ARG, QEMU_OPTION_writeconfig,
    "-writeconfig <file>\n"
    "                read/write config file\n", QEMU_ARCH_ALL)
STEXI
@item -writeconfig @var{file}
@findex -writeconfig
Write device configuration to @var{file}. The @var{file} can be either filename to save
command line and device configuration into file or dash @code{-}) character to print the
output to stdout. This can be later used as input file for @code{-readconfig} option.
ETEXI

DEF("no-user-config", 0, QEMU_OPTION_nouserconfig,
    "-no-user-config\n"
    "                do not load default user-provided config files at startup\n",
    QEMU_ARCH_ALL)
STEXI
@item -no-user-config
@findex -no-user-config
The @code{-no-user-config} option makes QEMU not load any of the user-provided
config files on @var{sysconfdir}.
ETEXI

DEF("trace", HAS_ARG, QEMU_OPTION_trace,
    "-trace [[enable=]<pattern>][,events=<file>][,file=<file>]\n"
    "                specify tracing options\n",
    QEMU_ARCH_ALL)
STEXI
HXCOMM This line is not accurate, as some sub-options are backend-specific but
HXCOMM HX does not support conditional compilation of text.
@item -trace [[enable=]@var{pattern}][,events=@var{file}][,file=@var{file}]
@findex -trace
@include qemu-option-trace.texi
ETEXI
DEF("plugin", HAS_ARG, QEMU_OPTION_plugin,
    "-plugin [file=]<file>[,arg=<string>]\n"
    "                load a plugin\n",
    QEMU_ARCH_ALL)
STEXI
@item -plugin file=@var{file}[,arg=@var{string}]
@findex -plugin

Load a plugin.

@table @option
@item file=@var{file}
Load the given plugin from a shared library file.
@item arg=@var{string}
Argument string passed to the plugin. (Can be given multiple times.)
@end table
ETEXI

HXCOMM Internal use
DEF("qtest", HAS_ARG, QEMU_OPTION_qtest, "", QEMU_ARCH_ALL)
DEF("qtest-log", HAS_ARG, QEMU_OPTION_qtest_log, "", QEMU_ARCH_ALL)

#ifdef __linux__
DEF("enable-fips", 0, QEMU_OPTION_enablefips,
    "-enable-fips    enable FIPS 140-2 compliance\n",
    QEMU_ARCH_ALL)
#endif
STEXI
@item -enable-fips
@findex -enable-fips
Enable FIPS 140-2 compliance mode.
ETEXI

HXCOMM Deprecated by -accel tcg
DEF("no-kvm", 0, QEMU_OPTION_no_kvm, "", QEMU_ARCH_I386)

DEF("msg", HAS_ARG, QEMU_OPTION_msg,
    "-msg timestamp[=on|off]\n"
    "                change the format of messages\n"
    "                on|off controls leading timestamps (default:on)\n",
    QEMU_ARCH_ALL)
STEXI
@item -msg timestamp[=on|off]
@findex -msg
prepend a timestamp to each log message.(default:on)
ETEXI

DEF("dump-vmstate", HAS_ARG, QEMU_OPTION_dump_vmstate,
    "-dump-vmstate <file>\n"
    "                Output vmstate information in JSON format to file.\n"
    "                Use the scripts/vmstate-static-checker.py file to\n"
    "                check for possible regressions in migration code\n"
    "                by comparing two such vmstate dumps.\n",
    QEMU_ARCH_ALL)
STEXI
@item -dump-vmstate @var{file}
@findex -dump-vmstate
Dump json-encoded vmstate information for current machine type to file
in @var{file}
ETEXI

DEF("enable-sync-profile", 0, QEMU_OPTION_enable_sync_profile,
    "-enable-sync-profile\n"
    "                enable synchronization profiling\n",
    QEMU_ARCH_ALL)
STEXI
@item -enable-sync-profile
@findex -enable-sync-profile
Enable synchronization profiling.
ETEXI

STEXI
@end table
ETEXI
DEFHEADING()

DEFHEADING(Generic object creation:)
STEXI
@table @option
ETEXI

DEF("object", HAS_ARG, QEMU_OPTION_object,
    "-object TYPENAME[,PROP1=VALUE1,...]\n"
    "                create a new object of type TYPENAME setting properties\n"
    "                in the order they are specified.  Note that the 'id'\n"
    "                property must be set.  These objects are placed in the\n"
    "                '/objects' path.\n",
    QEMU_ARCH_ALL)
STEXI
@item -object @var{typename}[,@var{prop1}=@var{value1},...]
@findex -object
Create a new object of type @var{typename} setting properties
in the order they are specified.  Note that the 'id'
property must be set.  These objects are placed in the
'/objects' path.

@table @option

@item -object memory-backend-file,id=@var{id},size=@var{size},mem-path=@var{dir},share=@var{on|off},discard-data=@var{on|off},merge=@var{on|off},dump=@var{on|off},prealloc=@var{on|off},host-nodes=@var{host-nodes},policy=@var{default|preferred|bind|interleave},align=@var{align}

Creates a memory file backend object, which can be used to back
the guest RAM with huge pages.

The @option{id} parameter is a unique ID that will be used to reference this
memory region when configuring the @option{-numa} argument.

The @option{size} option provides the size of the memory region, and accepts
common suffixes, eg @option{500M}.

The @option{mem-path} provides the path to either a shared memory or huge page
filesystem mount.

The @option{share} boolean option determines whether the memory
region is marked as private to QEMU, or shared. The latter allows
a co-operating external process to access the QEMU memory region.

The @option{share} is also required for pvrdma devices due to
limitations in the RDMA API provided by Linux.

Setting share=on might affect the ability to configure NUMA
bindings for the memory backend under some circumstances, see
Documentation/vm/numa_memory_policy.txt on the Linux kernel
source tree for additional details.

Setting the @option{discard-data} boolean option to @var{on}
indicates that file contents can be destroyed when QEMU exits,
to avoid unnecessarily flushing data to the backing file.  Note
that @option{discard-data} is only an optimization, and QEMU
might not discard file contents if it aborts unexpectedly or is
terminated using SIGKILL.

The @option{merge} boolean option enables memory merge, also known as
MADV_MERGEABLE, so that Kernel Samepage Merging will consider the pages for
memory deduplication.

Setting the @option{dump} boolean option to @var{off} excludes the memory from
core dumps. This feature is also known as MADV_DONTDUMP.

The @option{prealloc} boolean option enables memory preallocation.

The @option{host-nodes} option binds the memory range to a list of NUMA host
nodes.

The @option{policy} option sets the NUMA policy to one of the following values:

@table @option
@item @var{default}
default host policy

@item @var{preferred}
prefer the given host node list for allocation

@item @var{bind}
restrict memory allocation to the given host node list

@item @var{interleave}
interleave memory allocations across the given host node list
@end table

The @option{align} option specifies the base address alignment when
QEMU mmap(2) @option{mem-path}, and accepts common suffixes, eg
@option{2M}. Some backend store specified by @option{mem-path}
requires an alignment different than the default one used by QEMU, eg
the device DAX /dev/dax0.0 requires 2M alignment rather than 4K. In
such cases, users can specify the required alignment via this option.

The @option{pmem} option specifies whether the backing file specified
by @option{mem-path} is in host persistent memory that can be accessed
using the SNIA NVM programming model (e.g. Intel NVDIMM).
If @option{pmem} is set to 'on', QEMU will take necessary operations to
guarantee the persistence of its own writes to @option{mem-path}
(e.g. in vNVDIMM label emulation and live migration).
Also, we will map the backend-file with MAP_SYNC flag, which ensures the
file metadata is in sync for @option{mem-path} in case of host crash
or a power failure. MAP_SYNC requires support from both the host kernel
(since Linux kernel 4.15) and the filesystem of @option{mem-path} mounted
with DAX option.

@item -object memory-backend-ram,id=@var{id},merge=@var{on|off},dump=@var{on|off},share=@var{on|off},prealloc=@var{on|off},size=@var{size},host-nodes=@var{host-nodes},policy=@var{default|preferred|bind|interleave}

Creates a memory backend object, which can be used to back the guest RAM.
Memory backend objects offer more control than the @option{-m} option that is
traditionally used to define guest RAM. Please refer to
@option{memory-backend-file} for a description of the options.

@item -object memory-backend-memfd,id=@var{id},merge=@var{on|off},dump=@var{on|off},share=@var{on|off},prealloc=@var{on|off},size=@var{size},host-nodes=@var{host-nodes},policy=@var{default|preferred|bind|interleave},seal=@var{on|off},hugetlb=@var{on|off},hugetlbsize=@var{size}

Creates an anonymous memory file backend object, which allows QEMU to
share the memory with an external process (e.g. when using
vhost-user). The memory is allocated with memfd and optional
sealing. (Linux only)

The @option{seal} option creates a sealed-file, that will block
further resizing the memory ('on' by default).

The @option{hugetlb} option specify the file to be created resides in
the hugetlbfs filesystem (since Linux 4.14).  Used in conjunction with
the @option{hugetlb} option, the @option{hugetlbsize} option specify
the hugetlb page size on systems that support multiple hugetlb page
sizes (it must be a power of 2 value supported by the system).

In some versions of Linux, the @option{hugetlb} option is incompatible
with the @option{seal} option (requires at least Linux 4.16).

Please refer to @option{memory-backend-file} for a description of the
other options.

The @option{share} boolean option is @var{on} by default with memfd.

@item -object rng-builtin,id=@var{id}

Creates a random number generator backend which obtains entropy from
QEMU builtin functions. The @option{id} parameter is a unique ID that
will be used to reference this entropy backend from the @option{virtio-rng}
device. By default, the @option{virtio-rng} device uses this RNG backend.

@item -object rng-random,id=@var{id},filename=@var{/dev/random}

Creates a random number generator backend which obtains entropy from
a device on the host. The @option{id} parameter is a unique ID that
will be used to reference this entropy backend from the @option{virtio-rng}
device. The @option{filename} parameter specifies which file to obtain
entropy from and if omitted defaults to @option{/dev/urandom}.

@item -object rng-egd,id=@var{id},chardev=@var{chardevid}

Creates a random number generator backend which obtains entropy from
an external daemon running on the host. The @option{id} parameter is
a unique ID that will be used to reference this entropy backend from
the @option{virtio-rng} device. The @option{chardev} parameter is
the unique ID of a character device backend that provides the connection
to the RNG daemon.

@item -object tls-creds-anon,id=@var{id},endpoint=@var{endpoint},dir=@var{/path/to/cred/dir},verify-peer=@var{on|off}

Creates a TLS anonymous credentials object, which can be used to provide
TLS support on network backends. The @option{id} parameter is a unique
ID which network backends will use to access the credentials. The
@option{endpoint} is either @option{server} or @option{client} depending
on whether the QEMU network backend that uses the credentials will be
acting as a client or as a server. If @option{verify-peer} is enabled
(the default) then once the handshake is completed, the peer credentials
will be verified, though this is a no-op for anonymous credentials.

The @var{dir} parameter tells QEMU where to find the credential
files. For server endpoints, this directory may contain a file
@var{dh-params.pem} providing diffie-hellman parameters to use
for the TLS server. If the file is missing, QEMU will generate
a set of DH parameters at startup. This is a computationally
expensive operation that consumes random pool entropy, so it is
recommended that a persistent set of parameters be generated
upfront and saved.

@item -object tls-creds-psk,id=@var{id},endpoint=@var{endpoint},dir=@var{/path/to/keys/dir}[,username=@var{username}]

Creates a TLS Pre-Shared Keys (PSK) credentials object, which can be used to provide
TLS support on network backends. The @option{id} parameter is a unique
ID which network backends will use to access the credentials. The
@option{endpoint} is either @option{server} or @option{client} depending
on whether the QEMU network backend that uses the credentials will be
acting as a client or as a server. For clients only, @option{username}
is the username which will be sent to the server.  If omitted
it defaults to ``qemu''.

The @var{dir} parameter tells QEMU where to find the keys file.
It is called ``@var{dir}/keys.psk'' and contains ``username:key''
pairs.  This file can most easily be created using the GnuTLS
@code{psktool} program.

For server endpoints, @var{dir} may also contain a file
@var{dh-params.pem} providing diffie-hellman parameters to use
for the TLS server. If the file is missing, QEMU will generate
a set of DH parameters at startup. This is a computationally
expensive operation that consumes random pool entropy, so it is
recommended that a persistent set of parameters be generated
up front and saved.

@item -object tls-creds-x509,id=@var{id},endpoint=@var{endpoint},dir=@var{/path/to/cred/dir},priority=@var{priority},verify-peer=@var{on|off},passwordid=@var{id}

Creates a TLS anonymous credentials object, which can be used to provide
TLS support on network backends. The @option{id} parameter is a unique
ID which network backends will use to access the credentials. The
@option{endpoint} is either @option{server} or @option{client} depending
on whether the QEMU network backend that uses the credentials will be
acting as a client or as a server. If @option{verify-peer} is enabled
(the default) then once the handshake is completed, the peer credentials
will be verified. With x509 certificates, this implies that the clients
must be provided with valid client certificates too.

The @var{dir} parameter tells QEMU where to find the credential
files. For server endpoints, this directory may contain a file
@var{dh-params.pem} providing diffie-hellman parameters to use
for the TLS server. If the file is missing, QEMU will generate
a set of DH parameters at startup. This is a computationally
expensive operation that consumes random pool entropy, so it is
recommended that a persistent set of parameters be generated
upfront and saved.

For x509 certificate credentials the directory will contain further files
providing the x509 certificates. The certificates must be stored
in PEM format, in filenames @var{ca-cert.pem}, @var{ca-crl.pem} (optional),
@var{server-cert.pem} (only servers), @var{server-key.pem} (only servers),
@var{client-cert.pem} (only clients), and @var{client-key.pem} (only clients).

For the @var{server-key.pem} and @var{client-key.pem} files which
contain sensitive private keys, it is possible to use an encrypted
version by providing the @var{passwordid} parameter. This provides
the ID of a previously created @code{secret} object containing the
password for decryption.

The @var{priority} parameter allows to override the global default
priority used by gnutls. This can be useful if the system administrator
needs to use a weaker set of crypto priorities for QEMU without
potentially forcing the weakness onto all applications. Or conversely
if one wants wants a stronger default for QEMU than for all other
applications, they can do this through this parameter. Its format is
a gnutls priority string as described at
@url{https://gnutls.org/manual/html_node/Priority-Strings.html}.

@item -object filter-buffer,id=@var{id},netdev=@var{netdevid},interval=@var{t}[,queue=@var{all|rx|tx}][,status=@var{on|off}]

Interval @var{t} can't be 0, this filter batches the packet delivery: all
packets arriving in a given interval on netdev @var{netdevid} are delayed
until the end of the interval. Interval is in microseconds.
@option{status} is optional that indicate whether the netfilter is
on (enabled) or off (disabled), the default status for netfilter will be 'on'.

queue @var{all|rx|tx} is an option that can be applied to any netfilter.

@option{all}: the filter is attached both to the receive and the transmit
              queue of the netdev (default).

@option{rx}: the filter is attached to the receive queue of the netdev,
             where it will receive packets sent to the netdev.

@option{tx}: the filter is attached to the transmit queue of the netdev,
             where it will receive packets sent by the netdev.

@item -object filter-mirror,id=@var{id},netdev=@var{netdevid},outdev=@var{chardevid},queue=@var{all|rx|tx}[,vnet_hdr_support]

filter-mirror on netdev @var{netdevid},mirror net packet to chardev@var{chardevid}, if it has the vnet_hdr_support flag, filter-mirror will mirror packet with vnet_hdr_len.

@item -object filter-redirector,id=@var{id},netdev=@var{netdevid},indev=@var{chardevid},outdev=@var{chardevid},queue=@var{all|rx|tx}[,vnet_hdr_support]

filter-redirector on netdev @var{netdevid},redirect filter's net packet to chardev
@var{chardevid},and redirect indev's packet to filter.if it has the vnet_hdr_support flag,
filter-redirector will redirect packet with vnet_hdr_len.
Create a filter-redirector we need to differ outdev id from indev id, id can not
be the same. we can just use indev or outdev, but at least one of indev or outdev
need to be specified.

@item -object filter-rewriter,id=@var{id},netdev=@var{netdevid},queue=@var{all|rx|tx},[vnet_hdr_support]

Filter-rewriter is a part of COLO project.It will rewrite tcp packet to
secondary from primary to keep secondary tcp connection,and rewrite
tcp packet to primary from secondary make tcp packet can be handled by
client.if it has the vnet_hdr_support flag, we can parse packet with vnet header.

usage:
colo secondary:
-object filter-redirector,id=f1,netdev=hn0,queue=tx,indev=red0
-object filter-redirector,id=f2,netdev=hn0,queue=rx,outdev=red1
-object filter-rewriter,id=rew0,netdev=hn0,queue=all

@item -object filter-dump,id=@var{id},netdev=@var{dev}[,file=@var{filename}][,maxlen=@var{len}]

Dump the network traffic on netdev @var{dev} to the file specified by
@var{filename}. At most @var{len} bytes (64k by default) per packet are stored.
The file format is libpcap, so it can be analyzed with tools such as tcpdump
or Wireshark.

@item -object colo-compare,id=@var{id},primary_in=@var{chardevid},secondary_in=@var{chardevid},outdev=@var{chardevid},iothread=@var{id}[,vnet_hdr_support][,notify_dev=@var{id}]

Colo-compare gets packet from primary_in@var{chardevid} and secondary_in@var{chardevid}, than compare primary packet with
secondary packet. If the packets are same, we will output primary
packet to outdev@var{chardevid}, else we will notify colo-frame
do checkpoint and send primary packet to outdev@var{chardevid}.
In order to improve efficiency, we need to put the task of comparison
in another thread. If it has the vnet_hdr_support flag, colo compare
will send/recv packet with vnet_hdr_len.
If you want to use Xen COLO, will need the notify_dev to notify Xen
colo-frame to do checkpoint.

we must use it with the help of filter-mirror and filter-redirector.

@example

KVM COLO

primary:
-netdev tap,id=hn0,vhost=off,script=/etc/qemu-ifup,downscript=/etc/qemu-ifdown
-device e1000,id=e0,netdev=hn0,mac=52:a4:00:12:78:66
-chardev socket,id=mirror0,host=3.3.3.3,port=9003,server,nowait
-chardev socket,id=compare1,host=3.3.3.3,port=9004,server,nowait
-chardev socket,id=compare0,host=3.3.3.3,port=9001,server,nowait
-chardev socket,id=compare0-0,host=3.3.3.3,port=9001
-chardev socket,id=compare_out,host=3.3.3.3,port=9005,server,nowait
-chardev socket,id=compare_out0,host=3.3.3.3,port=9005
-object iothread,id=iothread1
-object filter-mirror,id=m0,netdev=hn0,queue=tx,outdev=mirror0
-object filter-redirector,netdev=hn0,id=redire0,queue=rx,indev=compare_out
-object filter-redirector,netdev=hn0,id=redire1,queue=rx,outdev=compare0
-object colo-compare,id=comp0,primary_in=compare0-0,secondary_in=compare1,outdev=compare_out0,iothread=iothread1

secondary:
-netdev tap,id=hn0,vhost=off,script=/etc/qemu-ifup,down script=/etc/qemu-ifdown
-device e1000,netdev=hn0,mac=52:a4:00:12:78:66
-chardev socket,id=red0,host=3.3.3.3,port=9003
-chardev socket,id=red1,host=3.3.3.3,port=9004
-object filter-redirector,id=f1,netdev=hn0,queue=tx,indev=red0
-object filter-redirector,id=f2,netdev=hn0,queue=rx,outdev=red1


Xen COLO

primary:
-netdev tap,id=hn0,vhost=off,script=/etc/qemu-ifup,downscript=/etc/qemu-ifdown
-device e1000,id=e0,netdev=hn0,mac=52:a4:00:12:78:66
-chardev socket,id=mirror0,host=3.3.3.3,port=9003,server,nowait
-chardev socket,id=compare1,host=3.3.3.3,port=9004,server,nowait
-chardev socket,id=compare0,host=3.3.3.3,port=9001,server,nowait
-chardev socket,id=compare0-0,host=3.3.3.3,port=9001
-chardev socket,id=compare_out,host=3.3.3.3,port=9005,server,nowait
-chardev socket,id=compare_out0,host=3.3.3.3,port=9005
-chardev socket,id=notify_way,host=3.3.3.3,port=9009,server,nowait
-object filter-mirror,id=m0,netdev=hn0,queue=tx,outdev=mirror0
-object filter-redirector,netdev=hn0,id=redire0,queue=rx,indev=compare_out
-object filter-redirector,netdev=hn0,id=redire1,queue=rx,outdev=compare0
-object iothread,id=iothread1
-object colo-compare,id=comp0,primary_in=compare0-0,secondary_in=compare1,outdev=compare_out0,notify_dev=nofity_way,iothread=iothread1

secondary:
-netdev tap,id=hn0,vhost=off,script=/etc/qemu-ifup,down script=/etc/qemu-ifdown
-device e1000,netdev=hn0,mac=52:a4:00:12:78:66
-chardev socket,id=red0,host=3.3.3.3,port=9003
-chardev socket,id=red1,host=3.3.3.3,port=9004
-object filter-redirector,id=f1,netdev=hn0,queue=tx,indev=red0
-object filter-redirector,id=f2,netdev=hn0,queue=rx,outdev=red1

@end example

If you want to know the detail of above command line, you can read
the colo-compare git log.

@item -object cryptodev-backend-builtin,id=@var{id}[,queues=@var{queues}]

Creates a cryptodev backend which executes crypto opreation from
the QEMU cipher APIS. The @var{id} parameter is
a unique ID that will be used to reference this cryptodev backend from
the @option{virtio-crypto} device. The @var{queues} parameter is optional,
which specify the queue number of cryptodev backend, the default of
@var{queues} is 1.

@example

 # @value{qemu_system} \
   [...] \
       -object cryptodev-backend-builtin,id=cryptodev0 \
       -device virtio-crypto-pci,id=crypto0,cryptodev=cryptodev0 \
   [...]
@end example

@item -object cryptodev-vhost-user,id=@var{id},chardev=@var{chardevid}[,queues=@var{queues}]

Creates a vhost-user cryptodev backend, backed by a chardev @var{chardevid}.
The @var{id} parameter is a unique ID that will be used to reference this
cryptodev backend from the @option{virtio-crypto} device.
The chardev should be a unix domain socket backed one. The vhost-user uses
a specifically defined protocol to pass vhost ioctl replacement messages
to an application on the other end of the socket.
The @var{queues} parameter is optional, which specify the queue number
of cryptodev backend for multiqueue vhost-user, the default of @var{queues} is 1.

@example

 # @value{qemu_system} \
   [...] \
       -chardev socket,id=chardev0,path=/path/to/socket \
       -object cryptodev-vhost-user,id=cryptodev0,chardev=chardev0 \
       -device virtio-crypto-pci,id=crypto0,cryptodev=cryptodev0 \
   [...]
@end example

@item -object secret,id=@var{id},data=@var{string},format=@var{raw|base64}[,keyid=@var{secretid},iv=@var{string}]
@item -object secret,id=@var{id},file=@var{filename},format=@var{raw|base64}[,keyid=@var{secretid},iv=@var{string}]

Defines a secret to store a password, encryption key, or some other sensitive
data. The sensitive data can either be passed directly via the @var{data}
parameter, or indirectly via the @var{file} parameter. Using the @var{data}
parameter is insecure unless the sensitive data is encrypted.

The sensitive data can be provided in raw format (the default), or base64.
When encoded as JSON, the raw format only supports valid UTF-8 characters,
so base64 is recommended for sending binary data. QEMU will convert from
which ever format is provided to the format it needs internally. eg, an
RBD password can be provided in raw format, even though it will be base64
encoded when passed onto the RBD sever.

For added protection, it is possible to encrypt the data associated with
a secret using the AES-256-CBC cipher. Use of encryption is indicated
by providing the @var{keyid} and @var{iv} parameters. The @var{keyid}
parameter provides the ID of a previously defined secret that contains
the AES-256 decryption key. This key should be 32-bytes long and be
base64 encoded. The @var{iv} parameter provides the random initialization
vector used for encryption of this particular secret and should be a
base64 encrypted string of the 16-byte IV.

The simplest (insecure) usage is to provide the secret inline

@example

 # @value{qemu_system} -object secret,id=sec0,data=letmein,format=raw

@end example

The simplest secure usage is to provide the secret via a file

 # printf "letmein" > mypasswd.txt
 # @value{qemu_system} -object secret,id=sec0,file=mypasswd.txt,format=raw

For greater security, AES-256-CBC should be used. To illustrate usage,
consider the openssl command line tool which can encrypt the data. Note
that when encrypting, the plaintext must be padded to the cipher block
size (32 bytes) using the standard PKCS#5/6 compatible padding algorithm.

First a master key needs to be created in base64 encoding:

@example
 # openssl rand -base64 32 > key.b64
 # KEY=$(base64 -d key.b64 | hexdump  -v -e '/1 "%02X"')
@end example

Each secret to be encrypted needs to have a random initialization vector
generated. These do not need to be kept secret

@example
 # openssl rand -base64 16 > iv.b64
 # IV=$(base64 -d iv.b64 | hexdump  -v -e '/1 "%02X"')
@end example

The secret to be defined can now be encrypted, in this case we're
telling openssl to base64 encode the result, but it could be left
as raw bytes if desired.

@example
 # SECRET=$(printf "letmein" |
            openssl enc -aes-256-cbc -a -K $KEY -iv $IV)
@end example

When launching QEMU, create a master secret pointing to @code{key.b64}
and specify that to be used to decrypt the user password. Pass the
contents of @code{iv.b64} to the second secret

@example
 # @value{qemu_system} \
     -object secret,id=secmaster0,format=base64,file=key.b64 \
     -object secret,id=sec0,keyid=secmaster0,format=base64,\
         data=$SECRET,iv=$(<iv.b64)
@end example

@item -object sev-guest,id=@var{id},cbitpos=@var{cbitpos},reduced-phys-bits=@var{val},[sev-device=@var{string},policy=@var{policy},handle=@var{handle},dh-cert-file=@var{file},session-file=@var{file}]

Create a Secure Encrypted Virtualization (SEV) guest object, which can be used
to provide the guest memory encryption support on AMD processors.

When memory encryption is enabled, one of the physical address bit (aka the
C-bit) is utilized to mark if a memory page is protected. The @option{cbitpos}
is used to provide the C-bit position. The C-bit position is Host family dependent
hence user must provide this value. On EPYC, the value should be 47.

When memory encryption is enabled, we loose certain bits in physical address space.
The @option{reduced-phys-bits} is used to provide the number of bits we loose in
physical address space. Similar to C-bit, the value is Host family dependent.
On EPYC, the value should be 5.

The @option{sev-device} provides the device file to use for communicating with
the SEV firmware running inside AMD Secure Processor. The default device is
'/dev/sev'. If hardware supports memory encryption then /dev/sev devices are
created by CCP driver.

The @option{policy} provides the guest policy to be enforced by the SEV firmware
and restrict what configuration and operational commands can be performed on this
guest by the hypervisor. The policy should be provided by the guest owner and is
bound to the guest and cannot be changed throughout the lifetime of the guest.
The default is 0.

If guest @option{policy} allows sharing the key with another SEV guest then
@option{handle} can be use to provide handle of the guest from which to share
the key.

The @option{dh-cert-file} and @option{session-file} provides the guest owner's
Public Diffie-Hillman key defined in SEV spec. The PDH and session parameters
are used for establishing a cryptographic session with the guest owner to
negotiate keys used for attestation. The file must be encoded in base64.

e.g to launch a SEV guest
@example
 # @value{qemu_system_x86} \
     ......
     -object sev-guest,id=sev0,cbitpos=47,reduced-phys-bits=5 \
     -machine ...,memory-encryption=sev0
     .....

@end example


@item -object authz-simple,id=@var{id},identity=@var{string}

Create an authorization object that will control access to network services.

The @option{identity} parameter is identifies the user and its format
depends on the network service that authorization object is associated
with. For authorizing based on TLS x509 certificates, the identity must
be the x509 distinguished name. Note that care must be taken to escape
any commas in the distinguished name.

An example authorization object to validate a x509 distinguished name
would look like:
@example
 # @value{qemu_system} \
     ...
     -object 'authz-simple,id=auth0,identity=CN=laptop.example.com,,O=Example Org,,L=London,,ST=London,,C=GB' \
     ...
@end example

Note the use of quotes due to the x509 distinguished name containing
whitespace, and escaping of ','.

@item -object authz-listfile,id=@var{id},filename=@var{path},refresh=@var{yes|no}

Create an authorization object that will control access to network services.

The @option{filename} parameter is the fully qualified path to a file
containing the access control list rules in JSON format.

An example set of rules that match against SASL usernames might look
like:

@example
  @{
    "rules": [
       @{ "match": "fred", "policy": "allow", "format": "exact" @},
       @{ "match": "bob", "policy": "allow", "format": "exact" @},
       @{ "match": "danb", "policy": "deny", "format": "glob" @},
       @{ "match": "dan*", "policy": "allow", "format": "exact" @},
    ],
    "policy": "deny"
  @}
@end example

When checking access the object will iterate over all the rules and
the first rule to match will have its @option{policy} value returned
as the result. If no rules match, then the default @option{policy}
value is returned.

The rules can either be an exact string match, or they can use the
simple UNIX glob pattern matching to allow wildcards to be used.

If @option{refresh} is set to true the file will be monitored
and automatically reloaded whenever its content changes.

As with the @code{authz-simple} object, the format of the identity
strings being matched depends on the network service, but is usually
a TLS x509 distinguished name, or a SASL username.

An example authorization object to validate a SASL username
would look like:
@example
 # @value{qemu_system} \
     ...
     -object authz-simple,id=auth0,filename=/etc/qemu/vnc-sasl.acl,refresh=yes
     ...
@end example

@item -object authz-pam,id=@var{id},service=@var{string}

Create an authorization object that will control access to network services.

The @option{service} parameter provides the name of a PAM service to use
for authorization. It requires that a file @code{/etc/pam.d/@var{service}}
exist to provide the configuration for the @code{account} subsystem.

An example authorization object to validate a TLS x509 distinguished
name would look like:

@example
 # @value{qemu_system} \
     ...
     -object authz-pam,id=auth0,service=qemu-vnc
     ...
@end example

There would then be a corresponding config file for PAM at
@code{/etc/pam.d/qemu-vnc} that contains:

@example
account requisite  pam_listfile.so item=user sense=allow \
           file=/etc/qemu/vnc.allow
@end example

Finally the @code{/etc/qemu/vnc.allow} file would contain
the list of x509 distingished names that are permitted
access

@example
CN=laptop.example.com,O=Example Home,L=London,ST=London,C=GB
@end example


@end table

ETEXI


HXCOMM This is the last statement. Insert new options before this line!
STEXI
@end table
ETEXI
