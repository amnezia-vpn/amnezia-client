#!/usr/bin/ruby

require 'xcodeproj'

class SSPatcher
	attr :project
	attr :target_main
	
	def run(file)
		open_project file
		open_target_main
		
		patch_main_target
		
		@project.save
	end
	
	def open_project(file)
		@project = Xcodeproj::Project.open(file)
		die 'Failed to open the project file: ' + file if @project.nil?
	end
	
	def open_target_main
		@target_main = @project.native_targets
						.select { |target| target.name == 'ShadowSocks' }
						.first
		return @target_main if not @target_main.nil?
		
		die 'Unable to open ShadowSocks target'
	end
	
	def patch_main_target
	    @target_main.resources_build_phase.files.each do |f| 
			puts f.display_name
			if f.display_name === "LICENSE"
				f.remove_from_project
			end
		end
	end
	
	def die(msg)
		print $msg
		exit 1
	end
end

if ARGV.length < 1
	puts "Usage: <script> project_file"
	exit 1
end


r = SSPatcher.new
r.run ARGV[0]
exit 0
