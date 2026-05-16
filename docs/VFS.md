BorealOS VFS specification 0.1 (first draft)
## Overview
The BorealOS VFS unifies multiple file systems under a single namespace. File systems are mounted under a named prefix. Each mount point can be accessed using `identifier:/path/to/file` where everything after the `:/` is the path on that file system.

#### Examples

- `initramfs:/` - the system's initial ram file system
- `system:/` - virtual system information filesystem
- `root:/` - the root partition
- `usb:/` - a mounted USB

## Path Syntax

A fully qualified VFS path has the the form:
```
<identifier>:/<path>
```
Where `<identifier>` is one or more characters (128 character limit, `[_a-zA-Z0-9]{1,128}`), and `<path>` a POSIX style path.

#### Rules

- Path separator is `/`
- `.` is the current directory
- `..` is the parent directory (if escaping the mount, it throws an error: `root:/../../`)
- Identifiers are case insensitive, while paths are case sensitive (depending on the underlying filesystem)
- A trailing `/` is ignored
- Consecutive `/` are collapsed (`root://some//file` becomes `root:/some/file`)

Relative paths (without `<identifier>:/`) are resolved against the current working directory of the calling process, which must be a fully qualified path.

## Mount System

Mounting is done through a config file.
A few mounts are always available, regardless of config.
Mounts are parsed linearly, and `system:/` is always available, to find block devices.
#### Config

Each new mount config must be separated by a newline, and each entry must follow the following syntax:
```
# This is a comment
mount <source> <target> [options] # this too
```

Where `<source>` is a VFS path, `<target>` is the mount point with the `:/` suffix, and options key value pairs using `key` or `key=value` syntax, separated by a space. Options can not have spaces in their value.

For example:
```
mount system:/devices/disks/sda1 root:/
mount system:/devices/disks/sda2 movies:/ noexec readonly 
```

The `root:/` prefix is expected to be there, if it is not available the system will not be able to function.

`<target>` must be unique, if the `<target>` collides with something, it tries to append a number like `<target>0`, and increments the number until no collision is found.
#### Options

The available options are:
- `readonly` - mounts the filesystem as readonly. Writes will fail.
- `noexec` - prevents execution of binaries on this mount.
- `sync` - writes are forced to be synchronous.
- `nocollide` - if the `<target>` collides, do not mount
- `required` - if the fs could not be mounted, error (produce a panic), if combined with `nocollide` produces a panic as well.

#### Mount Table

Under `system:/mounts/` there is a list of files where each entry corresponds to the `<identifier>:/` of a mount point. Each file contains information about the mount point.

Schema:
```
identifier = "root"
source = "<source>"
target = "root:/"
filesystem = "ext4"
options = ["readonly"]
mountedAt = 123 # unix timestamp
```

## `system:/` Filesystem

BorealOS exposes system information through the read-only mounted "system:/" virtual file system.
Information is exposed through files and folders, where each file has information exposed through the [TOML](https://toml.io/en/).

The default structure looks like:
```
system:/
	mounts/
		system
		initramfs
		temp
		user
		config
		usb
		xyz
		
	devices/
		disks/
			<id> # one file per block device
			<id>.partitions/ # if the device has partitions
				<partition0>
				<partition1>
			
		pci/
			<domain>:<bus>:<slot>.<func>/
				info
		
		cpu
		memory
		
	kernel/
		info
		cmdline
		modules/
			<name>
		
	processes/
		<pid>/
			info
			threads/
				<tid>/
			fds/
				<fd>
	
```

Where `cpu` would be something like:
```
count = 8
physicalCount = 4
processorName = "example processor"
vendorID = "example vendor"
features = ["sse", "avx2"]
baseClock = 3141 # (in mhz)
maxClock = 4600
```

The schemas for the other `system:/` components are TBD.

## Permissions
The VFS uses a unix like permission system, with UID, GID, and mode bits.
When the VFS encounters an operation that requires permissions, it asks the underlying FS to provide the permissions.

The FS implementer must decide what to do, when implementing the permission check function. If the FS does not support unix permissions (like FAT) it must decide on something.

The default approach should be read/write, unless otherwise necessary.

## Misc

The BorealOS VFS does not yet support symlinks, or hardlinks. This will be looked at later.

These are virtual file systems that provide other utilities.

#### `initramfs:/`
A read-only boot filesystem for loading modules & other stuff necessary for boot.
Unmounted after boot. Still reserved.

#### `temp:/`
A volatile memory backed temp folder, resets every boot.

#### `user:/`
Dynamically maps to the user's directory, would route to `root:/users/admin` for example, if the user is admin. This is based on the process's user ID.

#### `config:/`
System wide config files, these are read/write files stored in the `root:/system/config/`

## Errors

Out of bounds traversal (think `root:/../../../some/file`, checked per segment) returns `FS_OUT_OF_MOUNT`

Writing to a read only filesystem (like `system:/`) produces a FS_WRITE_PERMISSION error

#### List of errors
- `FS_SUCCESS` - 0, success!
- `FS_WRITE_PERMISSION` - 1, no write permission
- `FS_READ_PERMISSION` - 2, no read permission
- `FS_MISSING_MOUNT` - 3, no mount at provided identifier
- `FS_MISSING_ENTRY` - 4, no folder or file at provided path
- `FS_EXEC_PERMISSION` - 5, no exec permission
- `FS_OUT_OF_MOUNT` - 6, out of mount scope
- `FS_NOT_A_DIRECTORY` - 7, expected a directory but found a file
- `FS_ALREADY_EXISTS` - 8, file or folder already exists at path
- `FS_INVALID_PATH` - 9, path is invalid
- `FS_UNSUPPORTED` - 10, operation is unsupported by the underlying FS
- `FS_INVALID_DESCRIPTOR` - 11, file descriptor is invalid

These errors are returned as the function return code. (or syscall)

