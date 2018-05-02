#!/usr/bin/ruby

first_male_names = %W(Baltazar Bertrand Christen Kim Einar Karl)
first_female_names = %W(Andrea Andersine Lisbeth Lulu  Rigmor Sarina Sissel Tanja)
last_names = %W(Jensen Nielsen Hansen Pedersen Andersen Christensen  Larsen Rasmussen Petersen Madsen Kristensen Olsen Thomsen)
genders=%W(male female)
kommuner=%W(Silkeborg Gentofte Ballerup Odense Svendborg Lejre Vejle)

(1..100).each do |i|
  sex = genders[rand(genders.length)]
  if (sex == "male")
    first_name = first_male_names[rand(first_male_names.length)]
  else
    first_name = first_female_names[rand(first_female_names.length)]
  end
  last_name = last_names[rand(last_names.length)]
  age = rand(60)+20
  weight = rand(40)+40
  kommune = kommuner[rand(4)+rand(4)]
  if (sex == "male")
    weight += 20
  end
  puts "#{first_name} #{last_name} #{sex} #{age} #{weight} #{kommune}"
end
