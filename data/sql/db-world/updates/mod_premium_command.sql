-- .premium: opens the premium menu (see src/premium.cpp, premium_command). Player-level;
-- it requires an active row for the account in the characters database's `premium` table.
DELETE FROM `command` WHERE `name` = 'premium';
INSERT INTO `command` (`name`, `security`, `help`) VALUES
('premium', 0, 'Syntax: .premium\nOpens the premium menu. Requires a premium account.');
