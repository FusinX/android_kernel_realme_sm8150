obj-$(CONFIG_FS_ENCRYPTION)	+= fscrypto.o

ccflags-y += -Ifs/ext4
ccflags-y += -Ifs/f2fs

fscrypto-y := crypto.o fname.o hooks.o keyinfo.o policy.o
fscrypto-$(CONFIG_FS_ENCRYPTION_ICE) += fscrypt_ice.o
fscrypto-$(CONFIG_BLOCK) += bio.o
