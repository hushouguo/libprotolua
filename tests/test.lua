require('helper')

local o = {
	people = {
		{
			name = "hushouguo",
			id = 1,
			email = "kilimajaro@gmail.com",
			phones = {
				{
					number = "086-87654321",
					type = 2
				},
				{
					number = "086-12345678",
					type = 1
				}
			}
		}
	}
}
local s = protobuf_encode("AddressBook", o)
--print(s)
local oo = protobuf_decode("AddressBook", s)
dump(oo)



